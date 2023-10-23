#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <set>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

private:
    void MaybeEnlargePrintableSize(Size new_size);
    void MaybeShrinkPrintableSize(Position deleted_pos);

    void CheckPositionCorrectness(Position pos) const;

    void PrintCells(std::ostream& output,
                    const std::function<void(const CellInterface&)>& printCell) const;

    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;

    void DropCache(Position pos);
    void RecursiveCacheDrop(Position cur_pos, std::set<Position>& visited_vertices);
};