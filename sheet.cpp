#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text)
{
    CheckPositionCorrectness(pos);

    MaybeEnlargePrintableSize({pos.row + 1, pos.col + 1});

    if (cells_[pos.row][pos.col].get() == nullptr)
    {
        cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }
    DropCache(pos);
    cells_[pos.row][pos.col]->SetPos(pos);
    cells_[pos.row][pos.col]->Set(text);

}

const CellInterface* Sheet::GetCell(Position pos) const
{
    CheckPositionCorrectness(pos);
    if (pos.row > GetPrintableSize().rows - 1 || pos.col > GetPrintableSize().cols - 1)
    {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos)
{
    CheckPositionCorrectness(pos);
    if (pos.row > GetPrintableSize().rows - 1 || pos.col > GetPrintableSize().cols - 1)
    {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos)
{
    CheckPositionCorrectness(pos);
    DropCache(pos);
    if (pos.row < GetPrintableSize().rows && pos.col < GetPrintableSize().cols)
    {
        cells_[pos.row][pos.col] = nullptr;
        MaybeShrinkPrintableSize(pos);
    }


}

Size Sheet::GetPrintableSize() const
{
    if (cells_.empty())
    {
        return {0,0};
    }
    return {static_cast<int>(cells_.size()), static_cast<int>(cells_[0].size())};
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (const auto& row_vector : cells_)
    {
        for (auto it = row_vector.begin(); it != row_vector.end(); ++it)
        {
            if (it == row_vector.end() - 1 && *it == nullptr)
            {
                break;
            }

            if (*it == nullptr)
            {
                output << '\t';
                continue;
            }

            auto value = (*it)->GetValue();

            if (std::holds_alternative<std::string>(value))
            {
                output << std::get<std::string>(value);
            } else if (std::holds_alternative<double>(value))
            {
                output << std::get<double>(value);
            } else {
                output << std::get<FormulaError>(value);
            }

            if (it != row_vector.end() - 1)
            {
                output << '\t';
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const
{
    for (const auto& row_vector : cells_)
    {
        for (auto it = row_vector.begin(); it != row_vector.end(); ++it)
        {
            if (it == row_vector.end() - 1 && *it == nullptr)
            {
                break;
            }
            if (*it == nullptr)
            {
                output << '\t';
                continue;
            }
            output << (*it)->GetText();
            if (it != row_vector.end() - 1)
            {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::CheckPositionCorrectness(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position value");
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

const Cell* Sheet::GetConcreteCell(Position pos) const
{
    if (pos.row > GetPrintableSize().rows - 1 || pos.col > GetPrintableSize().cols - 1)
    {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}
Cell* Sheet::GetConcreteCell(Position pos)
{
    if (pos.row > GetPrintableSize().rows - 1 || pos.col > GetPrintableSize().cols - 1)
    {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

void Sheet::MaybeEnlargePrintableSize(Size new_size)
{
    if (cells_.empty())
    {
        cells_.reserve(new_size.rows);
        for(int i = 0; i < new_size.rows; ++i)
        {
            cells_.emplace_back(new_size.cols);
        }
        return;
    }


    while (static_cast<int>(cells_.size()) < new_size.rows)
    {
        cells_.emplace_back(cells_.front().size());
    }

    if (new_size.cols > static_cast<int>(cells_.front().size()))
    {
        for (auto& row : cells_)
        {
            while (static_cast<int>(row.size()) < new_size.cols)
            {
                row.emplace_back(nullptr);
            }
        }
    }
}
void Sheet::MaybeShrinkPrintableSize(Position deleted_pos)
{
    auto cur_sheet_size = GetPrintableSize();
    if (cur_sheet_size.rows == deleted_pos.row + 1)
    {
        while (!cells_.empty() && std::all_of(cells_.back().begin(), cells_.back().end(),
                                              [](const std::unique_ptr<Cell>& ptr )
                                              {return ptr.get() == nullptr; }))
        {
            cells_.pop_back();
        }
    }

    if (cells_.empty())
    {
        return;
    }


    if (cur_sheet_size.cols == deleted_pos.col + 1)
    {
        while(!cells_.front().empty())
        {
            bool delete_last_row = true;
            for (auto& row : cells_)
            {
                if (row.back().get() != nullptr)
                {
                    delete_last_row = false;
                }
            }
            if (delete_last_row)
            {
                for (auto& row : cells_)
                {
                    row.pop_back();
                }
            } else {
                break;
            }
        }

    }
}

void Sheet::RecursiveCacheDrop(Position cur_pos, std::set<Position>& visited_vertices)
{
    auto backward_refs = GetConcreteCell(cur_pos)->GetReferencedByCells();


    if (visited_vertices.count(cur_pos))
    {
        return;
    }
    GetConcreteCell(cur_pos)->ResetCache();

    visited_vertices.insert(cur_pos);
    for (const auto& ref : backward_refs)
    {
        RecursiveCacheDrop(ref, visited_vertices);
    }
}

void Sheet::DropCache(Position pos) {
    if (GetConcreteCell(pos) == nullptr)
    {
        return;
    }

    if (!GetConcreteCell(pos)->IsReferenced())
    {
        return;
    }

    std::set<Position> visited_vertices;

    RecursiveCacheDrop(pos, visited_vertices);


}
