#include <array>
#include <tuple>
#include <algorithm>
#include <iterator>


template<typename Type, typename CRTP> struct Strongly_Typed
{
  constexpr explicit Strongly_Typed(const Type val) noexcept : m_val(val) {}

  constexpr auto operator&(const Type rhs) const noexcept { return m_val & rhs; }

  friend constexpr auto operator&(const Type lhs, const CRTP rhs) noexcept { return lhs & rhs.m_val; }

  constexpr bool bit_set(const Type bit) const noexcept { return (m_val & (1 << bit)); }

protected:
  std::uint32_t m_val;
};

enum class Condition {
  EQ = 0b0000,  // Z set (equal)
  NE = 0b0001,  // Z clear (not equal)
  HS = 0b0010,
  CS = 0b0010,  // C set (unsigned higher or same)
  LO = 0b0011,
  CC = 0b0011,  // C clear (unsigned lower)
  MI = 0b0100,  // N set (negative)
  PL = 0b0101,  // N clear (positive or zero)
  VS = 0b0110,  // V set (overflow)
  VC = 0b0111,  // V clear (no overflow)
  HI = 0b1000,  // C set and Z clear (unsigned higher)
  LS = 0b1001,  // C clear or Z (set unsigned lower or same)
  GE = 0b1010,  // N set and V set, or N clear and V clear (> or =)
  LT = 0b1011,  // N set and V clear, or N clear and V set (>)
  GT = 0b1100,  // Z clear, and either N set and V set, or N clear and V set (>)
  LE = 0b1101,  // Z set, or N set and V clear,or N clear and V set (< or =)
  AL = 0b1110,  // Always
  NV = 0b1111   // Reserved
};

enum class OpCode {
  AND = 0b0000,  // Rd:= Op1 AND Op2
  EOR = 0b0001,  // Rd:= Op1 EOR Op2
  SUB = 0b0010,  // Rd:= Op1 - Op2
  RSB = 0b0011,  // Rd:= Op2 - Op1
  ADD = 0b0100,  // Rd:= Op1 + Op2
  ADC = 0b0101,  // Rd:= Op1 + Op2 + C
  SBC = 0b0110,  // Rd:= Op1 - Op2 + C
  RSC = 0b0111,  // Rd:= Op2 - Op1 + C
  TST = 0b1000,  // set condition codes on Op1 AND Op2
  TEQ = 0b1001,  // set condition codes on Op1 EOR Op2
  CMP = 0b1010,  // set condition codes on Op1 - Op2
  CMN = 0b1011,  // set condition codes on Op1 + Op2
  ORR = 0b1100,  // Rd:= Op1 OR Op2
  MOV = 0b1101,  // Rd:= Op2
  BIC = 0b1110,  // Rd:= Op1 AND NOT Op2
  MVN = 0b1111   // Rd:= NOT Op2
};

enum class Shift_Type { Logical_Left = 0b00, Logical_Right = 0b01, Arithmetic_Right = 0b10, Rotate_Right = 0b11 };


struct Instruction : Strongly_Typed<std::uint32_t, Instruction>
{
  using Strongly_Typed::Strongly_Typed;

  [[nodiscard]] constexpr Condition get_condition() const noexcept { return static_cast<Condition>(0b1111 & (m_val >> 28)); }

  friend struct Data_Processing;
};

struct Data_Processing : Strongly_Typed<std::uint32_t, Data_Processing>
{
  constexpr Data_Processing(Instruction ins) noexcept
    : Strongly_Typed{ ins.m_val } {}

        [[nodiscard]] constexpr OpCode get_opcode() const noexcept
  {
    return static_cast<OpCode>(0b1111 & (m_val >> 21));
  }

  [[nodiscard]] constexpr auto operand_2() const noexcept { return 0b1111'1111'1111 & m_val; }

  [[nodiscard]] constexpr auto operand_2_register() const noexcept { return 0b1111 & m_val; }

  [[nodiscard]] constexpr bool operand_2_immediate_shift() const noexcept { return !bit_set(4); }

  [[nodiscard]] constexpr auto operand_2_shift_register() const noexcept { return operand_2() >> 8; }

  [[nodiscard]] constexpr auto operand_2_shift_amount() const noexcept { return operand_2() >> 7; }

  [[nodiscard]] constexpr auto operand_2_shift_type() const noexcept { return static_cast<Shift_Type>((operand_2() >> 5) & 0b11); }

  [[nodiscard]] constexpr auto operand_2_immediate() const noexcept
  {
    const auto op_2         = operand_2();
    const std::int8_t value = 0b1111'1111 & op_2;

    // I believe this is defined behavior
    const auto sign_extended_value = static_cast<std::uint32_t>(static_cast<uint32_t>(value));

    const auto shift_right = (op_2 >> 8) * 2;

    // Avoiding UB for shifts >= size
    if (shift_right == 0 || shift_right == 32) {
      return static_cast<std::uint32_t>(sign_extended_value);
    }

    // Should create a ROR on any modern compiler.
    // no branching, CMOV on GCC
    return (sign_extended_value >> shift_right) | (sign_extended_value << (32 - shift_right));
  }

  constexpr auto destination_register() const noexcept { return 0b1111 & (m_val >> 12); }

  [[nodiscard]] constexpr auto operand_1_register() const noexcept { return 0b1111 & (m_val >> 16); }

  [[nodiscard]] constexpr bool set_condition_code() const noexcept { return bit_set(20); }

  [[nodiscard]] constexpr bool immediate_operand() const noexcept { return bit_set(25); }
};


enum class Instruction_Type {
  Branch_Exchange,
  Swap,
  Data_Processing,
  Multiply,
  Long_Multiply,
  Load_Store_Byte_Word,
  Load_Store_Multiple,
  Half_Word_Transfer_Immediate_Offset,
  Half_Word_Transfer_Register_Offset,
  Branch,
  Coprocessor_Data_Transfer,
  Coprocessor_Data_Operation,
  Coprocessor_Register_Transfer,
  Software_Interrupt,
  Unhandled_Opcode
};


[[nodiscard]] constexpr auto get_lookup_table() noexcept
{
  // hack for lack of constexpr std::bitset
  constexpr const auto bitcount = [](const auto &tuple) {
    auto value = std::get<0>(tuple);
    int count  = 0;
    while (value) {  // until all bits are zero
      count++;
      value &= (value - 1);
    }
    return count;
  };

  // hack for lack of constexpr swap
  constexpr const auto swap_tuples = [](auto &lhs, auto &rhs) noexcept
  {
    auto old         = lhs;

    std::get<0>(lhs) = std::get<0>(rhs);
    std::get<1>(lhs) = std::get<1>(rhs);
    std::get<2>(lhs) = std::get<2>(rhs);

    std::get<0>(rhs) = std::get<0>(old);
    std::get<1>(rhs) = std::get<1>(old);
    std::get<2>(rhs) = std::get<2>(old);
  };

  std::array<std::tuple<std::uint32_t, std::uint32_t, Instruction_Type>, 14> table{
    { { 0b0000'1100'0000'0000'0000'0000'0000'0000, 0b0000'0000'0000'0000'0000'0000'0000'0000, Instruction_Type::Data_Processing },
      { 0b0000'1111'1100'0000'0000'0000'1111'0000, 0b0000'0000'0000'0000'0000'0000'1001'0000, Instruction_Type::Multiply },
      { 0b0000'1111'1000'0000'0000'0000'1111'0000, 0b0000'0000'1000'0000'0000'0000'1001'0000, Instruction_Type::Long_Multiply },
      { 0b0000'1111'1011'0000'0000'1111'1111'0000, 0b0000'0001'0000'0000'0000'0000'1001'0000, Instruction_Type::Swap },
      { 0b0000'1100'0000'0000'0000'0000'0000'0000, 0b0000'0100'0000'0000'0000'0000'0000'0000, Instruction_Type::Load_Store_Byte_Word },
      { 0b0000'1110'0000'0000'0000'0000'0000'0000, 0b0000'1000'0000'0000'0000'0000'0000'0000, Instruction_Type::Load_Store_Multiple },
      { 0b0000'1110'0100'0000'0000'0000'1001'0000, 0b0000'0100'0000'0000'0000'0000'1001'0000, Instruction_Type::Half_Word_Transfer_Immediate_Offset },
      { 0b0000'1110'0100'0000'0000'1111'1001'0000, 0b0000'0000'0000'0000'0000'0000'1001'0000, Instruction_Type::Half_Word_Transfer_Register_Offset },
      { 0b0000'1110'0000'0000'0000'0000'0000'0000, 0b0000'1010'0000'0000'0000'0000'0000'0000, Instruction_Type::Branch },
      { 0b0000'1111'1111'1111'1111'1111'1111'0000, 0b0000'0001'0010'1111'1111'1111'0001'0000, Instruction_Type::Branch_Exchange },
      { 0b0000'1110'0000'0000'0000'0000'1111'0000, 0b0000'1100'0010'0000'0000'0000'0000'0000, Instruction_Type::Coprocessor_Data_Transfer },
      { 0b0000'1111'0000'0000'0000'0000'0001'0000, 0b0000'1110'0000'0000'0000'0000'0000'0000, Instruction_Type::Coprocessor_Data_Operation },
      { 0b0000'1111'0000'0000'0000'0000'0001'0000, 0b0000'1110'0000'0000'0000'0000'0001'0000, Instruction_Type::Coprocessor_Register_Transfer },
      { 0b0000'1111'0000'0000'0000'0000'0000'0000, 0b0000'1111'0000'0000'0000'0000'0000'0000, Instruction_Type::Software_Interrupt } }
  };

  // Order from most restrictive to least restrictive
  // Hack for lack of constexpr sort
  for (auto itr = begin(table); itr != end(table); ++itr) {
    swap_tuples(*itr, *std::min_element(itr, end(table), [bitcount](const auto &lhs, const auto &rhs) { return bitcount(lhs) > bitcount(rhs); }));
  }

  return table;
}


struct System
{
  std::uint32_t CSPR{};

  std::array<std::uint32_t, 16> registers{};

  [[nodiscard]] constexpr auto &PC() noexcept { return registers[15]; }
  [[nodiscard]] constexpr const auto &PC() const noexcept { return registers[15]; }

  constexpr void branch_exchange(const Instruction) noexcept;
  constexpr void swap(const Instruction) noexcept;

  constexpr auto get_second_operand_shift_amount(const Data_Processing val) const noexcept
  {
    if (val.operand_2_immediate_shift()) {
      return val.operand_2_shift_amount();
    } else {
      return 0xFF & registers[val.operand_2_shift_register()];
    }
  }

  [[nodiscard]] constexpr std::pair<bool, std::uint32_t>
    shift_register(const bool c_flag, const Shift_Type type, std::uint32_t shift_amount, std::uint32_t value) const noexcept
  {
    switch (type) {
    case Shift_Type::Logical_Left:
      if (shift_amount == 0) {
        return { c_flag, value };
      } else {
        return { value & (1 << (32 - shift_amount)), value << shift_amount };
      }
    case Shift_Type::Logical_Right:
      if (shift_amount == 0) {
        return { value & (1 << 31), 0 };
      } else {
        return { value & (1 << (shift_amount - 1)), value >> shift_amount };
      }
    case Shift_Type::Arithmetic_Right:
      if (shift_amount == 0) {
        const bool is_negative = value & (1 << 31);
        return { is_negative, is_negative ? -1 : 0 };
      } else {
        return { value & (1 << (shift_amount - 1)), (static_cast<std::int32_t>(value) >> shift_amount) };
      }
    case Shift_Type::Rotate_Right:
      if (shift_amount == 0) {
        // This is rotate_right_extended
        return { value & 1, (c_flag << 31) | (value >> 1) };
      } else {
        return { value & (1 << (shift_amount - 1)), (value >> shift_amount) | (value << (32 - shift_amount)) };
      }
    }
  }

  [[nodiscard]] constexpr std::pair<bool, std::int32_t> get_second_operand(const Data_Processing val) const noexcept
  {
    if (val.immediate_operand()) {
      return { c_flag(), val.operand_2_immediate() };
    } else {
      const auto shift_amount = get_second_operand_shift_amount(val);
      return shift_register(c_flag(), val.operand_2_shift_type(), shift_amount, registers[val.operand_2_register()]);
    }
  }

  constexpr void data_processing(const Data_Processing val) noexcept
  {
    const auto first_operand              = registers[val.operand_1_register()];
    const auto[carry_out, second_operand] = get_second_operand(val);
    const auto destination_register       = val.destination_register();
    auto &destination                     = registers[destination_register];

    const auto update_logical_flags = [ =, &destination, carry_out = carry_out, second_operand = second_operand ](const bool write, const auto result)
    {
      if (val.set_condition_code() && destination_register != 15) {
        c_flag(carry_out);
        z_flag(result == 0);
        n_flag(result & (1u << 31));
      }
      if (write) {
        destination = result;
      }
    };

    // use 64 bit operations to be able to capture carry
    const auto arithmetic =
      [ =, &destination, carry = c_flag(), first_operand = static_cast<std::uint64_t>(first_operand), second_operand = second_operand ](
        const bool write, const auto op)
    {
      const auto result = op(first_operand, second_operand, carry);

      static_assert(std::is_same_v<std::decay_t<decltype(result)>, std::uint64_t>);

      if (val.set_condition_code() && destination_register != 15) {
        z_flag(result == 0);
        n_flag(result & (1u << 31));
        c_flag(result & (1ull << 32));

        const auto first_op_sign  = static_cast<std::uint32_t>(first_operand) & (1 << 31);
        const auto second_op_sign = static_cast<std::uint32_t>(second_operand) & (1 << 31);
        const auto result_sign    = static_cast<std::uint32_t>(result) & (1 << 31);

        v_flag((first_op_sign == second_op_sign) && (result_sign != first_op_sign));
      }

      if (write) {
        destination = static_cast<std::uint32_t>(result);
      }
    };


    switch (val.get_opcode()) {
    // Logical Operations
    case OpCode::AND: update_logical_flags(true, first_operand & second_operand); break;
    case OpCode::EOR: update_logical_flags(true, first_operand ^ second_operand); break;
    case OpCode::TST: update_logical_flags(false, first_operand & second_operand); break;
    case OpCode::TEQ: update_logical_flags(false, first_operand ^ second_operand); break;
    case OpCode::ORR: update_logical_flags(true, first_operand | second_operand); break;
    case OpCode::MOV: update_logical_flags(true, second_operand); break;
    case OpCode::BIC: update_logical_flags(true, first_operand & (~second_operand)); break;
    case OpCode::MVN:
      update_logical_flags(true, ~second_operand);
      break;

    // Arithmetic Operations
    case OpCode::SUB: arithmetic(true, [](const auto op_1, const auto op_2, const auto) { return op_1 - op_2; }); break;
    case OpCode::RSB: arithmetic(true, [](const auto op_1, const auto op_2, const auto) { return op_2 - op_1; }); break;
    case OpCode::ADD: arithmetic(true, [](const auto op_1, const auto op_2, const auto) { return op_1 + op_2; }); break;
    case OpCode::ADC: arithmetic(true, [](const auto op_1, const auto op_2, const auto c) { return op_1 + op_2 + c; }); break;
    case OpCode::SBC: arithmetic(true, [](const auto op_1, const auto op_2, const auto c) { return op_1 - op_2 + c - 1; }); break;
    case OpCode::RSC: arithmetic(true, [](const auto op_1, const auto op_2, const auto c) { return op_2 - op_1 + c - 1; }); break;
    case OpCode::CMP: arithmetic(false, [](const auto op_1, const auto op_2, const auto) { return op_1 - op_2; }); break;
    case OpCode::CMN: arithmetic(false, [](const auto op_1, const auto op_2, const auto) { return op_1 + op_2; }); break;
    }
  }

  constexpr void multiply(const Instruction) noexcept;
  constexpr void long_multiply(const Instruction) noexcept;
  constexpr void load_store_byte_word(const Instruction) noexcept;
  constexpr void load_store_multiple(const Instruction) noexcept;
  constexpr void half_word_transfer_immediate_offset(const Instruction) noexcept;
  constexpr void half_word_transfer_register_offset(const Instruction) noexcept;

  constexpr void branch(const Instruction instruction) noexcept
  {
    if (instruction.bit_set(24)) {
      // Link bit set
      registers[14] = PC();
    }

    const auto offset = [](const auto &op) -> std::int32_t {
      if (op.bit_set(23)) {
        // is signed
        const auto twos_compliment = ((~(op & 0x00FFFFFFF)) + 1) & 0x00FFFFFF;
        return -(twos_compliment << 2);
      } else {
        return (op & 0x00FFFFFF) << 2;
      }
    }(instruction);

    PC() += offset + 4;
  }
  constexpr void coprocessor_data_transfer(const Instruction) noexcept;
  constexpr void coprocessor_data_operation(const Instruction) noexcept;
  constexpr void coprocessor_register_transfer(const Instruction) noexcept;
  constexpr void software_interrupt(const Instruction) noexcept;

  constexpr static auto n_bit = 0b1000'0000'0000'0000'0000'0000'0000'0000;
  constexpr static auto z_bit = 0b0100'0000'0000'0000'0000'0000'0000'0000;
  constexpr static auto c_bit = 0b0010'0000'0000'0000'0000'0000'0000'0000;
  constexpr static auto v_bit = 0b0001'0000'0000'0000'0000'0000'0000'0000;

  constexpr static void set_or_clear_bit(std::uint32_t &val, const std::uint32_t bit, const bool set)
  {
    if (set) {
      val |= bit;
    } else {
      val &= ~bit;
    }
  }


  constexpr bool n_flag() const noexcept { return CSPR & n_bit; }
  constexpr void n_flag(const bool val) noexcept { set_or_clear_bit(CSPR, n_bit, val); }

  constexpr bool z_flag() const noexcept { return CSPR & z_bit; }
  constexpr void z_flag(const bool val) noexcept { set_or_clear_bit(CSPR, z_bit, val); }

  constexpr bool c_flag() const noexcept { return CSPR & c_bit; }
  constexpr void c_flag(const bool val) noexcept { set_or_clear_bit(CSPR, c_bit, val); }

  constexpr bool v_flag() const noexcept { return CSPR & v_bit; }
  constexpr void v_flag(const bool val) noexcept { set_or_clear_bit(CSPR, v_bit, val); }

  //  constexpr void process_instruction
  [[nodiscard]] constexpr bool check_condition(const Instruction instruction) const noexcept
  {
    switch (instruction.get_condition()) {
    case Condition::EQ:  // Z set (==)
      return z_flag();
    case Condition::NE:  // Z clear (!=)
      return !z_flag();
    case Condition::HS:  // AKA CS C set (unsigned >=)
      return c_flag();
    case Condition::LO:  // AKA CC C clear (unsigned <)
      return !c_flag();
    case Condition::MI:  // N set (negative)
      return n_flag();
    case Condition::PL:  // N clear (positive or zero)
      return !n_flag();
    case Condition::VS:  // V set (overflow)
      return v_flag();
    case Condition::VC:  // V clear (no overflow)
      return !v_flag();
    case Condition::HI:  // C set and Z clear (unsigned higher)
      return c_flag() && !z_flag();
    case Condition::LS:  // C clear or Z (set unsigned lower or same)
      return !c_flag() && z_flag();
    case Condition::GE:  // N set and V set, or N clear and V clear (>=)
      return (n_flag() && v_flag()) || (!n_flag() && !v_flag());
    case Condition::LT:  // N set and V clear, or N clear and V set (<)
      return (n_flag() && !v_flag()) || (!n_flag() && v_flag());
    case Condition::GT:  // Z clear, and either N set and V set, or N clear and V clear (>)
      return !z_flag() && ((n_flag() && v_flag()) || (!n_flag() && !v_flag()));
    case Condition::LE:  // Z set, or N set and V clear,or N clear and V set (<=)
      return z_flag() || (n_flag() && !v_flag()) || (!n_flag() && v_flag());
    case Condition::AL:  // Always
      return true;
    case Condition::NV:  // Reserved
      return false;
    };
  }

  constexpr static auto lookup_table = get_lookup_table();

  [[nodiscard]] constexpr auto decode(const Instruction instruction) noexcept
  {
    for (const auto &elem : lookup_table) {
      if ((std::get<0>(elem) & instruction) == std::get<1>(elem)) {
        return std::get<2>(elem);
      }
    }

    return Instruction_Type::Unhandled_Opcode;
  }

  constexpr void process(const Instruction instruction) noexcept
  {
    PC() += 4;
    if (check_condition(instruction)) {
      switch (decode(instruction)) {
      case Instruction_Type::Data_Processing: data_processing(instruction); break;
      case Instruction_Type::Multiply: multiply(instruction); break;
      case Instruction_Type::Long_Multiply: long_multiply(instruction); break;
      case Instruction_Type::Swap: swap(instruction); break;
      case Instruction_Type::Load_Store_Byte_Word: load_store_byte_word(instruction); break;
      case Instruction_Type::Load_Store_Multiple: load_store_multiple(instruction); break;
      case Instruction_Type::Half_Word_Transfer_Immediate_Offset: half_word_transfer_immediate_offset(instruction); break;
      case Instruction_Type::Half_Word_Transfer_Register_Offset: half_word_transfer_register_offset(instruction); break;
      case Instruction_Type::Branch: branch(instruction); break;
      case Instruction_Type::Branch_Exchange: branch_exchange(instruction); break;
      case Instruction_Type::Coprocessor_Data_Transfer: coprocessor_data_transfer(instruction); break;
      case Instruction_Type::Coprocessor_Data_Operation: coprocessor_data_operation(instruction); break;
      case Instruction_Type::Coprocessor_Register_Transfer: coprocessor_register_transfer(instruction); break;
      case Instruction_Type::Software_Interrupt: software_interrupt(instruction); break;
      case Instruction_Type::Unhandled_Opcode: break;
      }
    }
  }
};

template<typename... T> constexpr auto run_instruction(T... instruction)
{
  System system;
  (system.process(instruction), ...);
  return system;
}


constexpr auto systest1 = run_instruction(Instruction{ 0b1111'1010'0000'0000'0000'0000'0000'1111 });
static_assert(systest1.PC() == 4);

constexpr auto systest2 = run_instruction(Instruction{ 0b1110'1010'0000'0000'0000'0000'0000'1111 });
static_assert(systest2.PC() == 68);
static_assert(systest2.registers[14] == 0);

constexpr auto systest3 = run_instruction(Instruction{ 0b1110'1011'0000'0000'0000'0000'0000'1111 });
static_assert(systest3.PC() == 68);
static_assert(systest3.registers[14] == 4);

constexpr auto systest4 = run_instruction(Instruction{ 0xe2800055 });  // add r0, r0, #85
static_assert(systest4.registers[0] == 0x55);

constexpr auto systest5 = run_instruction(Instruction{ 0xe2800055 },  // add r0, r0, #85
                                          Instruction{ 0xe2800c7e }   // add r0, r0, #32256
);
static_assert(systest5.registers[0] == (85 + 32256));


constexpr auto systest6 = run_instruction(Instruction{ 0xe2800001 },  // add r0, r0, #1
                                          Instruction{ 0xe2811009 },  // add r1, r1, #9
                                          Instruction{ 0xe2822002 },  // add r2, r2, #2
                                          Instruction{ 0xe0423001 }   // sub r3, r2, r1
);
static_assert(systest6.registers[3] == static_cast<std::uint32_t>(2 - 9));


constexpr auto systest7 = run_instruction(Instruction{ 0xe2800001 },  // add r0, r0, #1
                                          Instruction{ 0xe2811009 },  // add r1, r1, #9
                                          Instruction{ 0xe2822002 },  // add r2, r2, #2
                                          Instruction{ 0xe0403231 }   // sub r3, r0, r1, lsr r2
                                                                      // logical right shift r1 by the number in bottom byte of r2
                                                                      // subtract result from r0 and put answer in r3
);

static_assert(systest7.registers[3] == static_cast<std::uint32_t>(1 - (9 >> 2)));


static_assert(Instruction{ 0b1110'1010'0000'0000'0000'0000'0000'1111 }.get_condition() == Condition::AL);

int main(int argc, const char *[])
{
  System s;
  s.process(Instruction{ static_cast<std::uint32_t>(argc) });
}
