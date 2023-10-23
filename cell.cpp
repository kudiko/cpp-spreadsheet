#include "cell.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <utility>


class Cell::Impl
{
public:
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual ~Impl() = default;
};
class Cell::EmptyImpl : public Impl
{
public:
    EmptyImpl() = default;
    [[nodiscard]] Value GetValue() const override;
    [[nodiscard]] std::string GetText() const override;
    ~EmptyImpl() = default;
};
class Cell::TextImpl : public Impl
{
public:
    explicit TextImpl(std::string str);

    [[nodiscard]] Value GetValue() const override;

    [[nodiscard]] std::string GetText() const override;
    ~TextImpl() = default;

private:
    std::string text_;
};
class Cell::FormulaImpl : public Impl
{
public:
    explicit FormulaImpl(std::string str, SheetInterface& sheet);

    [[nodiscard]] Value GetValue() const override;

    [[nodiscard]] std::string GetText() const override;

    std::vector<Position> GetReferences() const;



    ~FormulaImpl() = default;
private:

    Formula formula_;
    SheetInterface& sheet_;
};

// Реализуйте следующие методы
Cell::Cell(SheetInterface& sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet)
{

}

Cell::~Cell() {}

void Cell::Set(std::string text)
{
    ManageDependencies();
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }

    if (!text.empty() && text.front() == FORMULA_SIGN && text.size() > 1)
    {
        ProcessSetFormulaCell(std::move(text));
        return;
    }

    impl_ = std::make_unique<TextImpl>(std::move(text));
}

void Cell::Clear()
{
    ManageDependencies();
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const
{
    if (HasCachedValue())
    {
        return cached_value_.value();
    }
    return impl_->GetValue();
}
std::string Cell::GetText() const
{
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return forward_dependencies_;
}

void Cell::AddReferencedCell(Position pos) {
    forward_dependencies_.push_back(pos);
    std::sort(forward_dependencies_.begin(), forward_dependencies_.end());
}

std::vector<Position> Cell::GetReferencedByCells() const {
    return backward_dependencies_;
}

void Cell::AddReferencedByCell(Position pos) {
    backward_dependencies_.push_back(pos);
    std::sort(backward_dependencies_.begin(), backward_dependencies_.end());
}

bool Cell::IsReferenced() const {
    return !backward_dependencies_.empty();
}

void Cell::ProcessSetFormulaCell(std::string text) {
//    try {
        std::unique_ptr<FormulaImpl> new_stuff = std::make_unique<FormulaImpl>(std::string{text.begin() + 1, text.end()}, sheet_);


        std::vector<Position> temp_forward_dependencies = GetReferencedCells();
        std::unique_ptr<Impl> temp = std::move(impl_);

        impl_ = std::move(new_stuff);
        forward_dependencies_ = dynamic_cast<FormulaImpl*>(impl_.get())->GetReferences();
        try {
            CheckCircularDependencies();
        } catch (const CircularDependencyException& ex) {
            impl_ = std::move(temp);
            forward_dependencies_ = std::move(temp_forward_dependencies);
            throw ex;
        }

//    } catch (...)
//    {
//        throw FormulaException("Invalid position: " + std::string{text.begin() + 1, text.end()});
//    }





    Sheet& sheet = dynamic_cast<Sheet&>(sheet_);
    for (const auto& pos : forward_dependencies_)
    {
        if (sheet.GetConcreteCell(pos) != nullptr)
        {
            sheet.GetConcreteCell(pos)->AddReferencedByCell(pos_);
        }
    }

    if (std::holds_alternative<double>(dynamic_cast<FormulaImpl*>(impl_.get())->GetValue()))
    {
        cached_value_ = std::get<double>(dynamic_cast<FormulaImpl*>(impl_.get())->GetValue());
    }



}

void Cell::SetPos(Position pos) {
    pos_ = pos;
}


[[nodiscard]] Cell::Value Cell::EmptyImpl::GetValue() const
    {
        return std::string{};
    }

    [[nodiscard]] std::string Cell::EmptyImpl::GetText() const
    {
        return {};
    }


    Cell::TextImpl::TextImpl(std::string str) : text_(std::move(str)) {}

    [[nodiscard]] Cell::Value Cell::TextImpl::GetValue() const
    {
        // text cell is not empty by definition, see Cell::Set
        if (text_.front() == ESCAPE_SIGN)
        {
            return std::string{text_.begin() + 1, text_.end()};
        }
        return text_;
    }

    [[nodiscard]] std::string Cell::TextImpl::GetText() const
    {
        return text_;
    }



    Cell::FormulaImpl::FormulaImpl(std::string str, SheetInterface& sheet) :
    formula_(std::move(str)), sheet_(sheet)
    {

    }


    [[nodiscard]] Cell::Value Cell::FormulaImpl::GetValue() const
    {


        auto result = formula_.Evaluate(sheet_);
        if (std::holds_alternative<FormulaError>(result))
        {
            return std::get<FormulaError>(result);
        }


        return std::get<double>(result);
    }

    [[nodiscard]] std::string Cell::FormulaImpl::GetText() const
    {
        return "=" + formula_.GetExpression();
    }

void Cell::ResetCache() const {
    cached_value_ = std::nullopt;
}

void Cell::EvaluateCache() const {
    if (!std::holds_alternative<double>(GetValue()))
    {
        return;
    }
    cached_value_ = std::get<double>(GetValue());
}

bool Cell::HasCachedValue() const {
    return cached_value_.has_value();
}


void Cell::ManageDependencies() {

    Sheet& sheet = dynamic_cast<Sheet&>(sheet_);
    for (const auto& pos : forward_dependencies_)
    {
        sheet.GetConcreteCell(pos)->RemoveReferencedByCell(pos_);
    }
    forward_dependencies_.clear();
}

void Cell::RemoveReferencedByCell(Position pos) {
    auto to_remove_it = std::lower_bound(backward_dependencies_.begin(),
                                         backward_dependencies_.end(), pos);
    backward_dependencies_.erase(to_remove_it);

}

void Cell::CheckCircularDependenciesRecursive(Position cur_pos, std::set<Position>& visited_vertices)
{
    Sheet& sheet = dynamic_cast<Sheet&>(sheet_);
    if (sheet.GetConcreteCell(cur_pos) == nullptr)
    {
        return;
    }
    auto forward_refs = sheet.GetConcreteCell(cur_pos)->forward_dependencies_;


    if (visited_vertices.count(cur_pos))
    {
        throw CircularDependencyException("Circular dependency detected");
    }

    visited_vertices.insert(cur_pos);
    for (const auto& ref : forward_refs)
    {
        CheckCircularDependenciesRecursive(ref, visited_vertices);
    }
}

void Cell::CheckCircularDependencies()
{
    std::set<Position> visited_vertices;

    CheckCircularDependenciesRecursive(pos_ , visited_vertices);
}

std::vector<Position> Cell::FormulaImpl::GetReferences() const
{
    return formula_.GetReferencedCells();
}

