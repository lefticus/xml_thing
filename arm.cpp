#include <array>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <iostream>

template<typename Type, typename CRTP> struct Strongly_Typed
{
  constexpr explicit Strongly_Typed(const Type val) noexcept : m_val(val) {}

  constexpr auto data() const noexcept
  {
    return m_val;
  }

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
  friend struct Single_Data_Transfer;
  friend struct Multiply_Long;
};

struct Single_Data_Transfer : Strongly_Typed<std::uint32_t, Single_Data_Transfer>
{
  constexpr Single_Data_Transfer(Instruction ins) noexcept
    : Strongly_Typed{ ins.m_val } {}


  [[nodiscard]] constexpr bool immediate_offset() const noexcept { return !bit_set(25); }

  [[nodiscard]] constexpr bool pre_indexing() const noexcept { return bit_set(24); }
  [[nodiscard]] constexpr bool up_indexing() const noexcept { return bit_set(23); }
  [[nodiscard]] constexpr bool byte_transfer() const noexcept { return bit_set(22); }
  [[nodiscard]] constexpr bool write_back() const noexcept { return bit_set(21); }
  [[nodiscard]] constexpr bool load() const noexcept { return bit_set(20); }

  [[nodiscard]] constexpr auto base_register() const noexcept { return (m_val >> 16) & 0b1111; }
  [[nodiscard]] constexpr auto src_dest_register() const noexcept { return (m_val >> 12) & 0b1111; }
  [[nodiscard]] constexpr auto offset() const noexcept { return m_val & 0xFFF; }
  [[nodiscard]] constexpr auto offset_register() const noexcept { return offset() & 0b1111; }
  [[nodiscard]] constexpr auto offset_shift() const noexcept { return offset() >> 4; }
  [[nodiscard]] constexpr auto offset_shift_type() const noexcept { return static_cast<Shift_Type>((offset_shift() >> 1) & 0b11); }
  [[nodiscard]] constexpr auto offset_shift_amount() const noexcept { return offset_shift() >> 4; }
};

struct Multiply_Long : Strongly_Typed<std::uint32_t, Multiply_Long>
{
  constexpr Multiply_Long(Instruction ins) noexcept
    : Strongly_Typed{ ins.m_val } {}


  [[nodiscard]] constexpr bool unsigned_mul() const noexcept { return !bit_set(22); }
  [[nodiscard]] constexpr bool accumulate() const noexcept { return !bit_set(21); }
  [[nodiscard]] constexpr bool status_register_update() const noexcept { return !bit_set(20); }
  [[nodiscard]] constexpr auto high_result() const noexcept { return (m_val >> 16) & 0b1111; }
  [[nodiscard]] constexpr auto low_result() const noexcept { return (m_val >> 12) & 0b1111; }
  [[nodiscard]] constexpr auto operand_1() const noexcept { return (m_val >> 8) & 0b1111; }
  [[nodiscard]] constexpr auto operand_2() const noexcept { return m_val & 0b1111; }

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
  Data_Processing,
  MRS,
  MSR,
  MSRF,
  Multiply,
  Multiply_Long,
  Single_Data_Swap,
  Single_Data_Transfer,
  Undefined,
  Block_Data_Transfer,
  Branch,
  Coprocessor_Data_Transfer,
  Coprocessor_Data_Operation,
  Coprocessor_Register_Transfer,
  Software_Interrupt
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
    auto old = lhs;

    std::get<0>(lhs) = std::get<0>(rhs);
    std::get<1>(lhs) = std::get<1>(rhs);
    std::get<2>(lhs) = std::get<2>(rhs);

    std::get<0>(rhs) = std::get<0>(old);
    std::get<1>(rhs) = std::get<1>(old);
    std::get<2>(rhs) = std::get<2>(old);
  };

  // ARMv3  http://netwinder.osuosl.org/pub/netwinder/docs/arm/ARM7500FEvB_3.pdf
  std::array<std::tuple<std::uint32_t, std::uint32_t, Instruction_Type>, 15> table{
    { { 0b0000'1100'0000'0000'0000'0000'0000'0000, 0b0000'0000'0000'0000'0000'0000'0000'0000, Instruction_Type::Data_Processing },
      { 0b0000'1111'1011'1111'0000'1111'1111'1111, 0b0000'0001'0000'1111'0000'1111'1111'1111, Instruction_Type::MRS },
      { 0b0000'1111'1011'1111'1111'1111'1111'0000, 0b0000'0001'0010'1001'1111'0000'0000'0000, Instruction_Type::MSR },
      { 0b0000'1101'1011'1111'1111'0000'0000'0000, 0b0000'0001'0010'1000'1111'0000'0000'0000, Instruction_Type::MSRF },
      { 0b0000'1111'1100'0000'0000'0000'1111'0000, 0b0000'0000'0000'0000'0000'0000'1001'0000, Instruction_Type::Multiply },
      { 0b0000'1111'1000'0000'0000'0000'1111'0000, 0b0000'0000'1000'0000'0000'0000'1001'0000, Instruction_Type::Multiply_Long },
      { 0b0000'1111'1011'0000'0000'1111'1111'0000, 0b0000'0001'0000'0000'0000'0000'1001'0000, Instruction_Type::Single_Data_Swap },
      { 0b0000'1100'0000'0000'0000'0000'0000'0000, 0b0000'0100'0000'0000'0000'0000'0000'0000, Instruction_Type::Single_Data_Transfer },
      { 0b0000'1110'0000'0000'0000'0000'0001'0000, 0b0000'0110'0000'0000'0000'0000'0001'0000, Instruction_Type::Undefined },
      { 0b0000'1110'0000'0000'0000'0000'0000'0000, 0b0000'1110'0000'0000'0000'0000'0000'0000, Instruction_Type::Block_Data_Transfer },
      { 0b0000'1110'0000'0000'0000'0000'0000'0000, 0b0000'1010'0000'0000'0000'0000'0000'0000, Instruction_Type::Branch },
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


template<std::size_t RAMSize = 1024> struct System
{
  std::uint32_t CSPR{};

  std::array<std::uint32_t, 16> registers{};
  std::array<std::uint8_t, RAMSize> RAM{};

  [[nodiscard]] constexpr auto &PC() noexcept { return registers[15]; }
  [[nodiscard]] constexpr const auto &PC() const noexcept { return registers[15]; }

  System() = default;

  template<std::size_t Size>
  constexpr System(const std::array<std::uint8_t, Size> &memory) noexcept
  {
    static_assert(Size <= RAMSize);

    // Workaround for missing constexpr copy (added in C++20, not in compilers yet)
    for (std::size_t loc = 0; loc < Size; ++loc) {
      RAM[loc] = memory[loc];
    }
  }

  constexpr Instruction get_instruction(const std::uint32_t PC) noexcept {
    // Note: there are more efficient ways to do this with reinterepet_cast
    // or an intrinsic, but I cannot with constexpr context and I see
    // no compiler that can optimize around this implementation
    const std::uint32_t byte_1 = RAM[PC];
    const std::uint32_t byte_2 = RAM[PC+1];
    const std::uint32_t byte_3 = RAM[PC+2];
    const std::uint32_t byte_4 = RAM[PC+3];

    return Instruction{byte_1 | (byte_2 << 8) | (byte_3 << 16) | (byte_4 << 24)};
  }

  constexpr void run(const std::uint32_t loc) noexcept
  {
    registers[14] = RAMSize;

    PC() = loc;
    while (PC() < RAMSize) {
      std::cout << std::hex << PC() << ':';
      for (const auto r : registers) {
        std::cout << ' ' << r;
      }
      std::cout << '\n';

      process(get_instruction(PC()));
    }
  }

  [[nodiscard]] constexpr auto get_second_operand_shift_amount(const Data_Processing val) const noexcept
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

  constexpr auto offset(const Single_Data_Transfer val) const noexcept {
    const auto offset = [=]() -> std::int32_t {
      if (val.immediate_offset()) {
        return val.offset();
      } else {
        const auto offset_register = registers[val.offset_register()];
        // note: carry out seems to have no use with Single_Data_Transfer
        return shift_register(c_flag(), val.offset_shift_type(), val.offset_shift_amount(), offset_register).second;
      }
    }();

    if (val.up_indexing()) {
      return offset;
    } else {
      return -offset;
    }
  }

  constexpr void single_data_transfer(const Single_Data_Transfer val) noexcept {
    const std::int32_t index_offset = offset(val);
    const auto base_location = registers[val.base_register()];
    const bool pre_indexed = val.pre_indexing();

    const auto src_dest_register = val.src_dest_register();
    const auto indexed_location = base_location + index_offset;

    if (val.byte_transfer()) {
      if (const auto location = pre_indexed ? indexed_location : base_location; val.load()) {
        registers[src_dest_register] = RAM[location];
      } else {
        RAM[location] = registers[src_dest_register];
      }
    } else {
      // word transfer
      if (const auto location = pre_indexed ? indexed_location : base_location; val.load()) {
        registers[src_dest_register] = static_cast<std::uint32_t>(RAM[location]) | (static_cast<std::uint32_t>(RAM[location + 1]) << 8) | (static_cast<std::uint32_t>(RAM[location + 2]) << 16) | (static_cast<std::uint32_t>(RAM[location + 3]) << 24);
      } else {
        RAM[location] = registers[src_dest_register] & 0xFF;
        RAM[location+1] = (registers[src_dest_register] >> 8) & 0xFF;
        RAM[location+2] = (registers[src_dest_register] >> 16) & 0xFF;
        RAM[location+3] = (registers[src_dest_register] >> 24) & 0xFF;
      }
    }

    if (!pre_indexed || val.write_back()) {
      registers[val.base_register()] = indexed_location;
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

  constexpr void multiply_long(const Multiply_Long val) noexcept
  {
    const auto result = [val, lhs = registers[val.operand_1()], rhs = registers[val.operand_2()]]() -> std::uint64_t {
      if (val.unsigned_mul()) {
        return static_cast<std::uint64_t>(lhs) * static_cast<std::uint64_t>(lhs);
      } else {
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(lhs) * static_cast<std::int64_t>(lhs));
      }
    }(); 

    if (val.accumulate()) {
      registers[val.high_result()] += (result >> 32) & 0xFFFFFFFF;
      registers[val.low_result()] += 0xFFFFFFFF;
    } else {
      registers[val.high_result()] = (result >> 32) & 0xFFFFFFFF;
      registers[val.low_result()] = 0xFFFFFFFF;
    }

    if (val.status_register_update()) {
      z_flag(result == 0);
      n_flag(result & (static_cast<std::uint64_t>(1) << 63));
    }
  }

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

    return Instruction_Type::Undefined;
  }

  constexpr void process(const Instruction instruction) noexcept
  {
    PC() += 4;
    if (check_condition(instruction)) {
      switch (decode(instruction)) {
      case Instruction_Type::Data_Processing: data_processing(instruction); break;
      case Instruction_Type::MRS: assert(!"MRS Not Implemented"); break;
      case Instruction_Type::MSR: assert(!"MSR Not Implemented"); break;
      case Instruction_Type::MSRF: assert(!"MSR flags Not Implemented"); break;
      case Instruction_Type::Multiply: assert(!"Multiply Not Implemented"); break;
      case Instruction_Type::Multiply_Long: multiply_long(instruction); break;
      case Instruction_Type::Single_Data_Swap: assert(!"Single_Data_Swap Not Implemented"); break;
      case Instruction_Type::Single_Data_Transfer: single_data_transfer(instruction); break;
      case Instruction_Type::Undefined: assert(!"Undefined Opcode"); break;
      case Instruction_Type::Block_Data_Transfer: assert(!"Block_Data_Transfer Not Implemented"); break;
      case Instruction_Type::Branch: branch(instruction); break;
      case Instruction_Type::Coprocessor_Data_Transfer: assert(!"Coprocessor_Data_Transfer Not Implemented"); break;
      case Instruction_Type::Coprocessor_Data_Operation: assert(!"Coprocessor_Data_Operation Not Implemented"); break;
      case Instruction_Type::Coprocessor_Register_Transfer: assert(!"Coprocessor_Register_Transfer Not Implemented"); break;
      case Instruction_Type::Software_Interrupt: assert(!"Software_Interrupt Not Implemented"); break;
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

template<typename ... T> constexpr auto run_code(std::uint32_t start, T ... byte)
{
  std::array<std::uint8_t, sizeof...(T)> memory{static_cast<std::uint8_t>(byte)...};
  System system{memory};
  system.run(start);
  return system;
}

void test_never_executing_jump()
{
  constexpr auto systest1 = run_instruction(Instruction{ 0b1111'1010'0000'0000'0000'0000'0000'1111 });
  static_assert(systest1.PC() == 4);
}

void test_always_executing_jump()
{
  constexpr auto systest2 = run_instruction(Instruction{ 0b1110'1010'0000'0000'0000'0000'0000'1111 });
  static_assert(systest2.PC() == 68);
  static_assert(systest2.registers[14] == 0);
}

void test_always_executing_jump_with_saved_return()
{
  constexpr auto systest3 = run_instruction(Instruction{ 0b1110'1011'0000'0000'0000'0000'0000'1111 });
  static_assert(systest3.PC() == 68);
  static_assert(systest3.registers[14] == 4);
}

void test_add_of_register()
{
  constexpr auto systest4 = run_instruction(Instruction{ 0xe2800055 });  // add r0, r0, #85
  static_assert(systest4.registers[0] == 0x55);
}

void test_add_of_register_with_shifts()
{
  constexpr auto systest5 = run_instruction(Instruction{ 0xe2800055 },  // add r0, r0, #85
                                            Instruction{ 0xe2800c7e }   // add r0, r0, #32256
  );
  static_assert(systest5.registers[0] == (85 + 32256));
}


void test_multiple_adds_and_sub()
{
  constexpr auto systest6 = run_instruction(Instruction{ 0xe2800001 },  // add r0, r0, #1
                                            Instruction{ 0xe2811009 },  // add r1, r1, #9
                                            Instruction{ 0xe2822002 },  // add r2, r2, #2
                                            Instruction{ 0xe0423001 }   // sub r3, r2, r1
  );
  static_assert(systest6.registers[3] == static_cast<std::uint32_t>(2 - 9));
}

void test_memory_writes()
{
  constexpr auto systest6 = run_instruction(Instruction{ 0xe3a00064 },  // mov r0, #100 ; 0x64
                                            Instruction{ 0xe3a01005 },  // mov r1, #5
                                            Instruction{ 0xe5c01000 },  // strb r1, [r0]
                                            Instruction{ 0xe3a00000 },  // mov r0, #0
                                            Instruction{ 0xe1a0f00e }   // mov pc, lr
  );

  static_assert(systest6.RAM[100] == 5);
}

void test_sub_with_shift()
{
  constexpr auto systest7 = run_instruction(Instruction{ 0xe2800001 },  // add r0, r0, #1
                                            Instruction{ 0xe2811009 },  // add r1, r1, #9
                                            Instruction{ 0xe2822002 },  // add r2, r2, #2
                                            Instruction{ 0xe0403231 }   // sub r3, r0, r1, lsr r2
                                                                        // logical right shift r1 by the number in bottom byte of r2
                                                                        // subtract result from r0 and put answer in r3
  );

  static_assert(systest7.registers[3] == static_cast<std::uint32_t>(1 - (9 >> 2)));
}

void test_looping()
{
  /*
  #include <cstdint>

  template<typename T>
    volatile T& get_loc(const std::uint32_t loc)
    {
      return *reinterpret_cast<T*>(loc);
    }

  int main()
  {
    for (int i = 0; i < 100; ++i) {
      get_loc<std::int8_t>(100+i) = i%5;
    }
  }
  */

  // compiles to:

  /*
  00000000 <main>:
   0:	e59f102c 	ldr	r1, [pc, #44]	; 34 <main+0x34>
   4:	e3a00000 	mov	r0, #0
   8:	e0832190 	umull	r2, r3, r0, r1
   c:	e1a02123 	lsr	r2, r3, #2
  10:	e0822102 	add	r2, r2, r2, lsl #2
  14:	e2622000 	rsb	r2, r2, #0
  18:	e0802002 	add	r2, r0, r2
  1c:	e5c02064 	strb	r2, [r0, #100]	; 0x64
  20:	e2800001 	add	r0, r0, #1
  24:	e3500064 	cmp	r0, #100	; 0x64
  28:	1afffff6 	bne	8 <main+0x8>
  2c:	e3a00000 	mov	r0, #0
  30:	e1a0f00e 	mov	pc, lr
  34:	cccccccd 	.word	0xcccccccd
  */

  auto system = run_code(0,
    0x2c, 0x10, 0x9f, 0xe5, 0x00, 0x00, 0xa0, 0xe3, 0x90, 0x21, 0x83, 0xe0, 0x23, 0x21, 0xa0, 0xe1, 0x02, 0x21, 0x82, 0xe0, 0x00, 0x20, 0x62, 0xe2, 0x02, 0x20, 0x80, 0xe0, 0x64, 0x20, 0xc0, 0xe5, 0x01, 0x00, 0x80, 0xe2, 0x64, 0x00, 0x50, 0xe3, 0xf6, 0xff, 0xff, 0x1a, 0x00, 0x00, 0xa0, 0xe3, 0x0e, 0xf0, 0xa0, 0xe1, 0xcd, 0xcc, 0xcc, 0xcc);

  std::cout << std::hex << static_cast<unsigned int>(system.RAM[0x34]) << '\n';

  std::cout << static_cast<int>(system.RAM[100]) << '\n';
//  static_assert(system.RAM[100] == 0);
//  static_assert(system.RAM[104] == 5);
//  static_assert(system.RAM[105] == 0);
//  static_assert(system.RAM[106] == 1);
}


void test_condition_parsing() { static_assert(Instruction{ 0b1110'1010'0000'0000'0000'0000'0000'1111 }.get_condition() == Condition::AL); }

int main(int argc, const char *[])
{
//  System s;
//  s.process(Instruction{ static_cast<std::uint32_t>(argc) });

  test_looping();
}

