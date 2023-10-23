#include "formula.h"

#include "common.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}



// Реализуйте следующие методы:
        Formula::Formula(std::string expression) : ast_(ParseFormulaAST(expression))
        {

        }
        Formula::Value Formula::Evaluate(const SheetInterface& sheet) const
        {
            Value result;

            auto pos_to_double = [&sheet](Position pos)
            {
                auto cell_interface_ptr = sheet.GetCell(pos);
                if (cell_interface_ptr == nullptr)
                {
                    SheetInterface& not_const_sheet_ref = const_cast<SheetInterface&>(sheet);
                    not_const_sheet_ref.SetCell(pos, "");
                    return 0.;
                }
                auto value = cell_interface_ptr->GetValue();

                if (std::holds_alternative<std::string>(value))
                {
                    auto string_value = std::get<std::string>(value);
                    if (string_value.empty())
                    {
                        return 0.;
                    }

                    double d;

                    size_t stod_end_pos = 0;
                    try {
                        d = std::stod(std::get<std::string>(value), &stod_end_pos);

                        if (stod_end_pos != string_value.size())
                        {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    catch (const std::invalid_argument& ex)
                    {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                    return d;
                }

                if (std::holds_alternative<FormulaError>(value))
                {
                    throw FormulaError(FormulaError::Category::Div0);
                }

                double numerical_value = std::get<double>(value);
                if (numerical_value == std::numeric_limits<double>::infinity() ||
                -numerical_value == std::numeric_limits<double>::infinity())
                {
                    throw FormulaError(FormulaError::Category::Div0);
                }

                return numerical_value;
            };

            try {
                result = ast_.Execute(pos_to_double);
            } catch (const FormulaError& formula_error) {
                return FormulaError(formula_error.GetCategory());
            }
            return result;
        }

        std::string Formula::GetExpression() const
        {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }

std::vector<Position> Formula::GetReferencedCells() const
{
    std::vector<Position> positions_from_ast{ast_.GetCells().begin(), ast_.GetCells().end()};
    auto new_end = std::unique(positions_from_ast.begin(), positions_from_ast.end());
    return {positions_from_ast.begin(), new_end};
}


std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

