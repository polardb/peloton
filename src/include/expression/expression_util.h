//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// expression_util.h
//
// Identification: src/include/expression/expression_util.h
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

#include "catalog/schema.h"
#include "catalog/catalog.h"
#include "expression/comparison_expression.h"
#include "expression/function_expression.h"
#include "expression/conjunction_expression.h"
#include "expression/constant_value_expression.h"
#include "expression/constant_value_expression.h"
#include "expression/operator_expression.h"
#include "expression/parameter_value_expression.h"
#include "expression/parser_expression.h"
#include "expression/string_expression.h"
#include "expression/tuple_value_expression.h"
#include "expression/tuple_value_expression.h"

namespace peloton {
namespace expression {

class ExpressionUtil {
 public:
  /**
   * This function generates a TupleValue expression from the column name
   */
  static AbstractExpression *ConvertToTupleValueExpression(
      catalog::Schema *schema, std::string column_name) {
    auto column_id = schema->GetColumnID(column_name);
    // If there is no column with this name
    if (column_id > schema->GetColumnCount()) {
      LOG_TRACE("Could not find column name %s in schema: %s",
                column_name.c_str(), schema->GetInfo().c_str());
      return nullptr;
    }
    LOG_TRACE("Column id in table: %u", column_id);
    expression::TupleValueExpression *expr =
        new expression::TupleValueExpression(schema->GetType(column_id), 0,
                                             column_id);
    return expr;
  }

  /**
   * This function converts each ParameterValueExpression in an expression tree
   * to
   * a value from the value vector
   */
  static void ConvertParameterExpressions(
      expression::AbstractExpression *expression, std::vector<Value> *values,
      catalog::Schema *schema) {
    LOG_TRACE("expression: %s", expression->GetInfo().c_str());

    if (expression->GetChild(0)) {
      LOG_TRACE("expression->left: %s",
                expression->GetChild(0)->GetInfo().c_str());
      if (expression->GetChild(0)->GetExpressionType() ==
          EXPRESSION_TYPE_VALUE_PARAMETER) {
        // left expression is parameter
        auto left = (ParameterValueExpression *)expression->GetChild(0);
        auto value =
            new ConstantValueExpression(values->at(left->GetValueIdx()));
        LOG_TRACE("left in vector type: %s",
                  values->at(left->GetValueIdx()).GetInfo().c_str());
        LOG_TRACE("Setting parameter %u to value %s", left->GetValueIdx(),
                  value->GetValue().GetInfo().c_str());
        expression->SetChild(0, value);
      } else {
        ConvertParameterExpressions(expression->GetModifiableChild(0), values,
                                    schema);
      }
    }

    if (expression->GetChild(1)) {
      LOG_TRACE("expression->right: %s",
                expression->GetChild(1)->GetInfo().c_str());
      if (expression->GetChild(1)->GetExpressionType() ==
          EXPRESSION_TYPE_VALUE_PARAMETER) {
        // right expression is parameter
        auto right = (ParameterValueExpression *)expression->GetChild(1);
        LOG_TRACE("right in vector type: %s",
                  values->at(right->GetValueIdx()).GetInfo().c_str());
        auto value =
            new ConstantValueExpression(values->at(right->GetValueIdx()));
        LOG_TRACE("Setting parameter %u to value %s", right->GetValueIdx(),
                  value->GetValue()
                      .GetInfo()
                      .c_str());
        expression->SetChild(1, value);
      } else {
        ConvertParameterExpressions(expression->GetModifiableChild(1), values,
                                    schema);
      }
    }
  }

  static AbstractExpression *TupleValueFactory(common::Type::TypeId value_type,
                                               const int tuple_idx,
                                               const int value_idx) {
    return new TupleValueExpression(value_type, tuple_idx, value_idx);
  }

  static AbstractExpression *ConstantValueFactory(const common::Value &value) {
    return new ConstantValueExpression(value);
  }

  static AbstractExpression *ComparisonFactory(ExpressionType type,
                                               AbstractExpression *left,
                                               AbstractExpression *right) {
    return new ComparisonExpression(type, left, right);
  }

  static AbstractExpression *ConjunctionFactory(ExpressionType type,
                                                AbstractExpression *left,
                                                AbstractExpression *right) {
    return new ConjunctionExpression(type, left, right);
  }

  static AbstractExpression *OperatorFactory(ExpressionType type,
                                             common::Type::TypeId value_type,
                                             AbstractExpression *left,
                                             AbstractExpression *right) {
    return new OperatorExpression(type, value_type, left, right);
  }

  static AbstractExpression *OperatorFactory(
      ExpressionType type, common::Type::TypeId value_type,
      AbstractExpression *expr1, AbstractExpression *expr2,
      AbstractExpression *expr3, UNUSED_ATTRIBUTE AbstractExpression *expr4) {
    switch (type) {
      case (EXPRESSION_TYPE_ASCII):
        return new AsciiExpression(expr1);
      case (EXPRESSION_TYPE_CHAR):
        return new ChrExpression(expr1);
      case (EXPRESSION_TYPE_SUBSTR):
        return new SubstrExpression(expr1, expr2, expr3);
      case (EXPRESSION_TYPE_CHAR_LEN):
        return new CharLengthExpression(expr1);
      case (EXPRESSION_TYPE_CONCAT):
        return new ConcatExpression(expr1, expr2);
      case (EXPRESSION_TYPE_OCTET_LEN):
        return new OctetLengthExpression(expr1);
      case (EXPRESSION_TYPE_REPEAT):
        return new RepeatExpression(expr1, expr2);
      case (EXPRESSION_TYPE_REPLACE):
        return new ReplaceExpression(expr1, expr2, expr3);
      case (EXPRESSION_TYPE_LTRIM):
        return new LTrimExpression(expr1, expr2);
      case (EXPRESSION_TYPE_RTRIM):
        return new RTrimExpression(expr1, expr2);
      case (EXPRESSION_TYPE_BTRIM):
        return new BTrimExpression(expr1, expr2);
      default:
        return new OperatorExpression(type, value_type, expr1, expr2);
    }
  }

  static AbstractExpression *ConjunctionFactory(
      ExpressionType type,
      const std::list<expression::AbstractExpression *> &child_exprs) {
    if (child_exprs.empty())
      return new ConstantValueExpression(common::ValueFactory::GetBooleanValue(1));
    AbstractExpression *left = nullptr;
    AbstractExpression *right = nullptr;
    for (auto expr : child_exprs) {
      left = expr;
      right = new ConjunctionExpression(type, left, right);
    }
    return left;
  }

  inline static bool IsAggregateExpression(ExpressionType type) {
    switch (type) {
    case EXPRESSION_TYPE_AGGREGATE_COUNT:
    case EXPRESSION_TYPE_AGGREGATE_COUNT_STAR:
    case EXPRESSION_TYPE_AGGREGATE_SUM:
    case EXPRESSION_TYPE_AGGREGATE_MIN:
    case EXPRESSION_TYPE_AGGREGATE_MAX:
    case EXPRESSION_TYPE_AGGREGATE_AVG:
    case EXPRESSION_TYPE_AGGREGATE_APPROX_COUNT_DISTINCT:
    case EXPRESSION_TYPE_AGGREGATE_VALS_TO_HYPERLOGLOG:
    case EXPRESSION_TYPE_AGGREGATE_HYPERLOGLOGS_TO_CARD:
      return true;
    default:
      return false;
    }
  }

  inline static bool IsOperatorExpression(ExpressionType type) {
    switch (type) {
    case EXPRESSION_TYPE_AGGREGATE_COUNT:
    case EXPRESSION_TYPE_AGGREGATE_COUNT_STAR:
    case EXPRESSION_TYPE_AGGREGATE_SUM:
    case EXPRESSION_TYPE_AGGREGATE_MIN:
    case EXPRESSION_TYPE_AGGREGATE_MAX:
    case EXPRESSION_TYPE_AGGREGATE_AVG:
    case EXPRESSION_TYPE_AGGREGATE_APPROX_COUNT_DISTINCT:
    case EXPRESSION_TYPE_AGGREGATE_VALS_TO_HYPERLOGLOG:
    case EXPRESSION_TYPE_AGGREGATE_HYPERLOGLOGS_TO_CARD:
      return true;
    default:
      return false;
    }
  }

  /**
   * This function replaces all COLUMN_REF expressions with TupleValue
   * expressions
   */
  static void TransformExpression(
      catalog::Schema *schema, AbstractExpression *expr) {
    bool dummy;
    TransformExpression(nullptr, nullptr, expr, schema, dummy, false);
  }

  static void TransformExpression(std::unordered_map<oid_t, oid_t> &column_mapping, std::vector<oid_t> &column_ids,
        AbstractExpression *expr, const catalog::Schema& schema, bool &needs_projection) {
    TransformExpression(&column_mapping, &column_ids, expr, &schema, needs_projection, true);
  }

 private:
  static void TransformExpression(std::unordered_map<oid_t, oid_t> *column_mapping, std::vector<oid_t> *column_ids,
       AbstractExpression *expr, const catalog::Schema* schema, bool &needs_projection, bool find_columns) {
    if (expr == nullptr){
      return;
    }
    size_t num_children = expr->GetChildrenSize();
    for(size_t child = 0; child < num_children; child++){
      TransformExpression(column_mapping, column_ids, expr->GetModifiableChild(child), schema, needs_projection, find_columns);
    }
    if (expr->GetExpressionType() == EXPRESSION_TYPE_VALUE_TUPLE && expr->GetValueType() == Type::INVALID) {
      auto val_expr = (expression::TupleValueExpression *)expr;
      auto col_id = schema->GetColumnID(val_expr->col_name_);
      if (col_id == (oid_t)-1){
        throw Exception("Column "+val_expr->col_name_ +" not found");
      }
      auto column = schema->GetColumn(col_id);

      size_t mapped_position;
      if (find_columns){
        if (column_mapping->count(col_id) == 0){
          mapped_position = column_ids->size();
          column_ids->push_back(col_id);
          (*column_mapping)[col_id] = mapped_position;
        }else{
          mapped_position = (*column_mapping)[col_id];
        }
      }else{
        mapped_position = col_id;
      }
      auto type = column.GetType();
      if (val_expr->alias.size() > 0){
        val_expr->expr_name_ = val_expr->alias;
      }else{
        val_expr->expr_name_ = val_expr->col_name_;
      }
      val_expr->SetTupleValueExpressionParams(type, mapped_position, 0);
    }else if (expr->GetExpressionType() != EXPRESSION_TYPE_STAR){
      needs_projection = true;
    }

    if (expr->GetExpressionType() == EXPRESSION_TYPE_FUNCTION){
      auto func_expr = (expression::FunctionExpression*)expr;
      auto  catalog = catalog::Catalog::GetInstance();
      catalog::FunctionData func_data = catalog->GetFunction(func_expr->func_name_);
      func_expr->SetFunctionExpressionParameters(func_data.func_ptr_, func_data.return_type_);
    }
    expr->DeduceExpressionType();
  }

};
}
}
