#pragma once

#include "common.h"


#include <functional>
#include <unordered_set>
#include <optional>
#include <set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);

    ~Cell();

    void Set(std::string text);

    void Clear();

    Value GetValue() const override;

    std::string GetText() const override;

    // forward ref
    std::vector<Position> GetReferencedCells() const override;
    void AddReferencedCell(Position pos);

    // backward ref
    std::vector<Position> GetReferencedByCells() const;
    void AddReferencedByCell(Position pos);
    void RemoveReferencedByCell(Position pos);

    bool IsReferenced() const;

    void SetPos(Position pos);

    void ResetCache() const;
    void EvaluateCache() const;
    bool HasCachedValue() const;

private:
    class Impl;

    class EmptyImpl;

    class TextImpl;

    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    std::vector<Position> backward_dependencies_;
    std::vector<Position> forward_dependencies_;

    Position pos_;
    SheetInterface& sheet_;

    mutable std::optional<double> cached_value_ = std::nullopt;

    void ProcessSetFormulaCell(std::string text);
    void ManageDependencies();
    void CheckCircularDependencies();
    void CheckCircularDependenciesRecursive(Position cur_pos, std::set<Position>& visited_vertices);
};
// Добавьте поля и методы для связи с таблицей, проверки циклических
// зависимостей, графа зависимостей и т. д.