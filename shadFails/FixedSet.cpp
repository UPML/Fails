#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <random>

template<typename T>
class Optional {
public:
    Optional &operator=(const T &value);

    bool IsAssigned() const;

    T GetValue() const;

private:
    bool assigned_ = false;
    T value_;
};

std::vector<int> ReadVector(std::istream &in);

struct Hash {
    explicit Hash(size_t multiplier_value = 0, size_t adder_value = 0);

    size_t operator()(int value) const;

private:
    size_t multiplier_value;
    size_t adder_valuer;
    static const size_t kPrimeNumber = 2000000011;
};

class HashFactory {
public:
    static HashFactory &Instance();

    explicit HashFactory(HashFactory const &) = delete;

    HashFactory &operator=(HashFactory const &) = delete;

    Hash GetNextHash();

private:
    HashFactory();

    std::mt19937 random_generator_;
};

template<typename T, typename Hash, typename HashFactory>
class FixedSet {
public:
    FixedSet() : inner_data_size_(0), is_initialized_(false) {}

    void Initialize(std::vector<T> data);

    bool Contains(const T &value) const;

protected:
    size_t CalcInnerPosition(const T &value) const;

    size_t inner_data_size_;

    std::vector<size_t> CalcDistribution(const std::vector<T> &data);

private:
    virtual void InitBufferAndSize(size_t size) = 0;

    virtual bool TryFillingHashTable(const std::vector<T> &data) = 0;

    virtual bool HasKey(const T &value) const = 0;

    bool is_initialized_;
    Hash hash_;
};

template<typename T, typename Hash, typename HashFactory>
class PerfectHashFirstLevelHashTable : public FixedSet<T, Hash, HashFactory> {
private:
    std::vector<Optional<T>> inner_data_;

    void InitBufferAndSize(size_t size) override;

    bool HasKey(const T &value) const override;

    bool TryFillingHashTable(const std::vector<T> &data) override;
};


template<typename T, typename Hash, typename HashFactory>
class PerfectHashTable : public FixedSet<T, Hash, HashFactory> {
private:
    std::vector<PerfectHashFirstLevelHashTable<T, Hash, HashFactory>> hashTable_;

    void InitBufferAndSize(size_t size) override;

    bool HasKey(const T &value) const override;

    bool TryFillingHashTable(const std::vector<T> &data) override;

    size_t kMemoryRepletionRatio = 4;
};

void OperateQueries(const std::vector<int> &queries,
                    const PerfectHashTable<int, Hash, HashFactory> &static_hash_table);

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    auto data = ReadVector(std::cin);
    auto queries = ReadVector(std::cin);
    PerfectHashTable<int, Hash, HashFactory> static_hash_table;
    static_hash_table.Initialize(data);
    OperateQueries(queries, static_hash_table);
    return 0;
}

template<typename T>
Optional<T> &Optional<T>::operator=(const T &value) {
    value_ = value;
    assigned_ = true;
    return *this;
}

template<typename T>
bool Optional<T>::IsAssigned() const {
    return assigned_;
}

template<typename T>
T Optional<T>::GetValue() const {
    assert(assigned_);
    return value_;
}

Hash::Hash(size_t multiplier_value, size_t adder_value) :
        multiplier_value(multiplier_value),
        adder_valuer(adder_value) {}

size_t Hash::operator()(int value) const {
    return (value * multiplier_value + adder_valuer) % kPrimeNumber;
}

HashFactory &HashFactory::Instance() {
    static HashFactory hashFactory;
    return hashFactory;
}

Hash HashFactory::GetNextHash() {
    return Hash(random_generator_(), random_generator_());
}

HashFactory::HashFactory() { random_generator_.seed(42); }

std::vector<int> ReadVector(std::istream &in) {
    size_t size;
    in >> size;
    std::vector<int> data(size);
    for (size_t i = 0; i < size; ++i) {
        in >> data[i];
    }
    return data;
}

void OperateQueries(const std::vector<int> &queries,
                    const PerfectHashTable<int, Hash, HashFactory> &static_hash_table) {
    for (auto value: queries) {
        if (static_hash_table.Contains(value)) {
            std::cout << "Yes\n";
        } else {
            std::cout << "No\n";
        }
    }
}

template<typename T, typename Hash, typename HashFactory>
void FixedSet<T, Hash, HashFactory>::Initialize(std::vector<T> data) {
    InitBufferAndSize(data.size());
    HashFactory &hashFactory = HashFactory::Instance();
    hash_ = hashFactory.GetNextHash();
    while (!TryFillingHashTable(data)) {
        hash_ = hashFactory.GetNextHash();
    }
    is_initialized_ = true;
}

template<typename T, typename Hash, typename HashFactory>
bool FixedSet<T, Hash, HashFactory>::Contains(const T &value) const {
    assert(is_initialized_);
    if (inner_data_size_ == 0) {
        return false;
    }
    return HasKey(value);
}

template<typename T, typename Hash, typename HashFactory>
size_t FixedSet<T, Hash, HashFactory>::CalcInnerPosition(const T &value) const {
    return hash_(value) % inner_data_size_;
}

template<typename T, typename Hash, typename HashFactory>
std::vector<size_t> FixedSet<T, Hash, HashFactory>::CalcDistribution(const std::vector<T> &data) {
    std::vector<size_t> baskets(inner_data_size_, 0);
    for (auto value : data) {
        ++baskets[CalcInnerPosition(value)];
    }
    return baskets;
}

template<typename T, typename Hash, typename HashFactory>
void PerfectHashFirstLevelHashTable<T, Hash, HashFactory>::InitBufferAndSize(size_t size) {
    this->inner_data_size_ = size * size;
    inner_data_.resize(this->inner_data_size_);
}

template<typename T, typename Hash, typename HashFactory>
bool PerfectHashFirstLevelHashTable<T, Hash, HashFactory>::HasKey(const T &value) const {
    auto optional_value = inner_data_[this->CalcInnerPosition(value)];
    return optional_value.IsAssigned() && optional_value.GetValue() == value;
}

template<typename T, typename Hash, typename HashFactory>
bool PerfectHashFirstLevelHashTable<T, Hash, HashFactory>::TryFillingHashTable(
        const std::vector<T> &data) {
    auto distribution = this->CalcDistribution(data);
    for (auto num : distribution) {
        if (num > 1) {
            return false;
        }
    }
    for (auto value: data) {
        inner_data_[this->CalcInnerPosition(value)] = value;
    }
    return true;
}


template<typename T, typename Hash, typename HashFactory>
void PerfectHashTable<T, Hash, HashFactory>::InitBufferAndSize(size_t size) {
    this->inner_data_size_ = size;
    hashTable_.resize(this->inner_data_size_);
}

template<typename T, typename Hash, typename HashFactory>
bool PerfectHashTable<T, Hash, HashFactory>::HasKey(const T &value) const {
    return hashTable_[this->CalcInnerPosition(value)].Contains(value);
}

template<typename T, typename Hash, typename HashFactory>
bool PerfectHashTable<T, Hash, HashFactory>::TryFillingHashTable(const std::vector<T> &data) {
    auto distribution = this->CalcDistribution(data);
    size_t sum_size = 0;
    for (auto &number: distribution) {
        sum_size += number * number;
    }
    if (sum_size > kMemoryRepletionRatio * hashTable_.size()) {
        return false;
    } else {
        std::vector<std::vector<T>> baskets(hashTable_.size());
        assert(distribution.size() == hashTable_.size());
        for (size_t i = 0; i < hashTable_.size(); ++i) {
            baskets[i].reserve(distribution[i]);
        }
        for (auto value : data) {
            baskets[this->CalcInnerPosition(value)].push_back(value);
        }

        for (size_t i = 0; i < hashTable_.size(); ++i) {
            hashTable_[i].Initialize(baskets[i]);
        }
        return true;
    }
}
