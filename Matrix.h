#include <vector>
#include <optional>
#include <initializer_list>
#include <iostream>

template <typename T>
class Matrix {
private:
    std::vector<T> data;

public:
    unsigned int rows;
    unsigned int columns;

public:
    Matrix(unsigned int rows, unsigned int columns) : rows(rows), columns(columns) {
        data.resize(rows * columns);
    }

    Matrix(std::initializer_list<std::initializer_list<T>> init) {
        rows = init.size();
        columns = init.begin()->size();
        data.reserve(rows * columns);

        for (const auto& row : init) {
            if (row.size() != columns) {
                throw std::invalid_argument("All rows must have the same number of columns");
            }
            data.insert(data.end(), row.begin(), row.end());
        }
    }

    // constructor for scalar * initializer list
    Matrix(T scalar, std::initializer_list<std::initializer_list<T>> init) {
        rows = init.size();
        columns = init.begin()->size();
        data.reserve(rows * columns);

        for (const auto& row : init) {
            if (row.size() != columns) {
                throw std::invalid_argument("All rows must have the same number of columns");
            }
            for (const auto& elem : row) {
                data.push_back(scalar * elem);
            }
        }
    }

    std::optional<T> get_element(unsigned int row, unsigned int col) const {
        if (row >= rows || col >= columns) {
            return std::nullopt;
        }
        return data[row * columns + col];
    }

    bool set_element(unsigned int row, unsigned int col, const T& value) {
        if (row >= rows || col >= columns) {
            return false;
        }
        data[row * columns + col] = value;
        return true;
    }

    unsigned int get_rows() const { return rows; }
    unsigned int get_columns() const { return columns; }

    // Matrix * scalar
    Matrix operator*(const T& scalar) const {
        Matrix result(rows, columns);
        for (unsigned int i = 0; i < rows * columns; ++i) {
            result.data[i] = data[i] * scalar;
        }
        return result;
    }

    // Matrix *= scalar
    Matrix& operator*=(const T& scalar) {
        for (unsigned int i = 0; i < rows * columns; ++i) {
            data[i] *= scalar;
        }
        return *this;
    }

    void print() const {
        for (unsigned int i = 0; i < rows; ++i) {
            for (unsigned int j = 0; j < columns; ++j) {
                if (auto element = get_element(i, j)) {
                    std::cout << *element << " ";
                }
            }
            std::cout << std::endl;
        }
    }
};

// Operator overload for scalar * matrix
template <typename T>
Matrix<T> operator*(const T& scalar, const Matrix<T>& matrix) {
    Matrix<T> result(matrix.get_rows(), matrix.get_columns());
    for (unsigned int i = 0; i < matrix.get_rows(); ++i) {
        for (unsigned int j = 0; j < matrix.get_columns(); ++j) {
            if (auto element = matrix.get_element(i, j)) {
                result.set_element(i, j, scalar * (*element));
            }
        }
    }
    return result;
}
