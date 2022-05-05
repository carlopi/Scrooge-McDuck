#include "aggregations/aggregate.hpp"
#include "duckdb/function/function_set.hpp"
#include "duckdb/parser/parsed_data/create_aggregate_function_info.hpp"

namespace scrooge {

template <class T> struct FirstScroogeState {
  T first;
  int64_t earliest_time;
  bool executed;
};

struct FirstScroogeOperation {
  template <class STATE> static void Initialize(STATE *state) {
    state->earliest_time = duckdb::NumericLimits<int64_t>::Maximum();
    state->executed = false;
  }

  template <class A_TYPE, class B_TYPE, class STATE, class OP>
  static void Operation(STATE *state, duckdb::FunctionData *bind_data,
                        A_TYPE *x_data, B_TYPE *y_data,
                        duckdb::ValidityMask &amask,
                        duckdb::ValidityMask &bmask, idx_t xidx, idx_t yidx) {

    const auto time = y_data[yidx];
    if (!state->executed || time < state->earliest_time) {
      state->earliest_time = time;
      state->first = x_data[xidx];
      state->executed = true;
    }
  }

  template <class STATE, class OP>
  static void Combine(const STATE &source, STATE *target,
                      duckdb::FunctionData *bind_data) {
    if (!target->executed) {
      *target = source;
    } else if (source.executed) {
      if (target->earliest_time > source.earliest_time) {
        target->earliest_time = source.earliest_time;
        target->first = source.first;
      }
    }
  }

  template <class T, class STATE>
  static void Finalize(duckdb::Vector &result, duckdb::FunctionData *,
                       STATE *state, T *target, duckdb::ValidityMask &mask,
                       idx_t idx) {
    if (!state->executed) {
      mask.SetInvalid(idx);
    } else {
      target[idx] = state->first;
    }
  }

  static bool IgnoreNull() { return true; }
};

std::unique_ptr<duckdb::FunctionData> BindDoubleFirst(
    duckdb::ClientContext &context, duckdb::AggregateFunction &bound_function,
    std::vector<duckdb::unique_ptr<duckdb::Expression>> &arguments) {
  auto &decimal_type = arguments[0]->return_type;
  switch (decimal_type.InternalType()) {
  case duckdb::PhysicalType::INT16: {
    bound_function =
        duckdb::AggregateFunction::BinaryAggregate<FirstScroogeState<int16_t>,
                                                   int16_t, int64_t, int16_t,
                                                   FirstScroogeOperation>(
            decimal_type, duckdb::LogicalType::TIMESTAMP_TZ, decimal_type);
    break;
  }
  case duckdb::PhysicalType::INT32: {
    bound_function =
        duckdb::AggregateFunction::BinaryAggregate<FirstScroogeState<int32_t>,
                                                   int32_t, int64_t, int32_t,
                                                   FirstScroogeOperation>(
            decimal_type, duckdb::LogicalType::TIMESTAMP_TZ, decimal_type);
    break;
  }
  case duckdb::PhysicalType::INT64: {
    bound_function =
        duckdb::AggregateFunction::BinaryAggregate<FirstScroogeState<int64_t>,
                                                   int64_t, int64_t, int64_t,
                                                   FirstScroogeOperation>(
            decimal_type, duckdb::LogicalType::TIMESTAMP_TZ, decimal_type);
    break;
  }
  default:
    bound_function = duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<duckdb::Hugeint>, duckdb::Hugeint, int64_t,
        duckdb::Hugeint, FirstScroogeOperation>(
        decimal_type, duckdb::LogicalType::TIMESTAMP_TZ, decimal_type);
  }
  bound_function.name = "first_s";
  return nullptr;
}

duckdb::AggregateFunction
GetFirstScroogeFunction(const duckdb::LogicalType &type) {
  switch (type.id()) {
  case duckdb::LogicalTypeId::SMALLINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<int16_t>, int16_t, int64_t, int16_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::TINYINT:
    return duckdb::AggregateFunction::BinaryAggregate<FirstScroogeState<int8_t>,
                                                      int8_t, int64_t, int8_t,
                                                      FirstScroogeOperation>(
        type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::INTEGER:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<int32_t>, int32_t, int64_t, int32_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::BIGINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<int64_t>, int64_t, int64_t, int64_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::DECIMAL: {
    auto decimal_aggregate = duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<duckdb::hugeint_t>, duckdb::hugeint_t, int64_t,
        duckdb::hugeint_t, FirstScroogeOperation>(
        type, duckdb::LogicalType::TIMESTAMP_TZ, type);
    decimal_aggregate.bind = BindDoubleFirst;
    return decimal_aggregate;
  }
  case duckdb::LogicalTypeId::FLOAT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<float>, float, int64_t, float, FirstScroogeOperation>(
        type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::DOUBLE:
    return duckdb::AggregateFunction::BinaryAggregate<FirstScroogeState<double>,
                                                      double, int64_t, double,
                                                      FirstScroogeOperation>(
        type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::UTINYINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<uint8_t>, uint8_t, int64_t, uint8_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::USMALLINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<uint16_t>, uint16_t, int64_t, uint16_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::UINTEGER:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<uint32_t>, uint32_t, int64_t, uint32_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::UBIGINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<uint64_t>, uint64_t, int64_t, uint64_t,
        FirstScroogeOperation>(type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  case duckdb::LogicalTypeId::HUGEINT:
    return duckdb::AggregateFunction::BinaryAggregate<
        FirstScroogeState<duckdb::hugeint_t>, duckdb::hugeint_t, int64_t,
        duckdb::hugeint_t, FirstScroogeOperation>(
        type, duckdb::LogicalType::TIMESTAMP_TZ, type);
  default:
    throw duckdb::InternalException(
        "Scrooge First Function only accept Numeric Inputs");
  }
}

void FirstScrooge::RegisterFunction(duckdb::Connection &conn,
                                    duckdb::Catalog &catalog) {
  // The first aggregate allows you to get the first value of one column as
  // ordered by another e.g., first(temperature, time) returns the earliest
  // temperature value based on time within an aggregate group.
  duckdb::AggregateFunctionSet first("first_s");
  for (auto &type : duckdb::LogicalType::Numeric()) {
    first.AddFunction(GetFirstScroogeFunction(type));
  }
  duckdb::CreateAggregateFunctionInfo first_info(first);
  catalog.CreateFunction(*conn.context, &first_info);
}

} // namespace scrooge