#pragma once

#include <storages/postgres/detail/typed_rows.hpp>
#include <storages/postgres/result_set.hpp>

namespace storages::postgres {

/// @page psql_typed_results Typed PostgreSQL results.
///
/// The ResultSet provides access to a generic PostgreSQL result buffer wrapper
/// with access to individual column buffers and means to parse the buffers into
/// a certain type.
///
/// For a user that wishes to get the results in a form of a sequence or a
/// container of C++ tuples or structures, the driver provides a way to coerce
/// the generic result set into a typed result set or a container of tuples or
/// structures that fulfil certain conditions.
///
/// TypedResultSet provides container interface for typed result rows for
/// iteration or random access without converting all the result set at once.
/// The iterators in the TypedResultSet satisfy requirements for a constant
/// RandomAccessIterator with the exception of dereferencing iterators. The
/// prefix dereferencing operator of the iterators returns value (not a
/// reference to it) and the iterators don't have the postfix dereferencing
/// operators (->).
///
/// # Data row coercion
///
/// The data rows can be obtained as:
///   - std::tuple;
///   - aggregate class as is;
///   - non-aggregate class with some augmentation.
///
/// Members of the tuple or the classes must have appropriate PostgreSQL data
/// parsers.
/// For more information about parsers see @ref psql_io (@todo add psql_io page,
/// will be completed within https://st.yandex-team.ru/TAXICOMMON-363).
///
/// For more information about adding user types see @ref psql_user_types (@todo
/// add psql_user_types page, will be completed within
/// https://st.yandex-team.ru/TAXICOMMON-363).
///
/// ## std::tuple.
///
/// The first option is to convert ResultSet's row to std::tuples.
///
/// ```
/// using my_row_type = std::tuple<int, string>;
/// auto trx = ...;
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.As<my_row_type>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), my_row_type>,
///       "Iterate over tuples");
///   auto [a, b] = row;
///   std::cout << "a = " << a << "; b = " << b << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<my_row_type>>();
/// ```
///
/// ## Aggregate classes.
///
/// A data row can be coerced to an aggregate class.
///
/// An aggregate class (C++03 8.5.1 §1) is a class that with no base classes, no
/// protected or private non-static data members, no user-declared constructors
/// and no virtual functions.
///
/// ```
/// struct my_row_type {
///   int a;
///   std::string b;
/// };
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.As<my_row_type>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), my_row_type>,
///       "Iterate over aggregate classes");
///   std::cout << "a = " << row.a << "; b = " << row.b << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<my_row_type>>();
/// ```
///
/// ## Non-aggregate classes.
///
/// Classes that do not satisfy the aggregate class requirements can be used
/// to be created from data rows by providing additional `Introspect` non-static
/// member function. The function should return a tuple of references to
/// member data fields. The class must be default constructible.
///
/// ```
/// class my_row_type {
///  private:
///   int a_;
///   std::string b_;
///  public:
///   auto Introspect() {
///     return std::tie(a_, b_);
///   }
///   int GetA() const;
///   const std::string& GetB() const;
/// };
///
/// auto generic_result = trx.Execute("select a, b from my_table");
/// auto iteration = generic_result.As<my_row_type>();
/// for (auto row : iteration) {
///   static_assert(std::is_same_v<decltype(row), my_row_type>,
///       "Iterate over non-aggregate classes");
///   std::cout << "a = " << row.GetA() << "; b = " << row.GetB() << "\n";
/// }
///
/// auto data = geric_result.AsContainer<std::vector<my_row_type>>();
/// ```
template <typename T>
class TypedResultSet {
 public:
  using size_type = ResultSet::size_type;
  using difference_type = ResultSet::difference_type;
  static constexpr size_type npos = ResultSet::npos;

  //@{
  /** @name Row container concept */
  using const_iterator = detail::ConstTypedRowIterator<T>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using value_type = T;
  using reference = const value_type;
  using pointer = const_iterator;
  //@}
 private:
  //@{
  using UserData = detail::RowToUserData<T>;
  //@}
 public:
  explicit TypedResultSet(ResultSet result) : result_{result} {}

  /// Number of rows in the result set
  size_type Size() const { return result_.Size(); }
  bool IsEmpty() const { return Size() == 0; }
  explicit operator bool() const { return !IsEmpty(); }
  bool operator!() const { return IsEmpty(); }
  //@{
  /** @name Container interface */
  //@{
  /** @name Row container interface */
  //@{
  /** @name Forward iteration */
  const_iterator cbegin() const { return const_iterator{result_.pimpl_, 0}; }
  const_iterator begin() const { return cbegin(); }
  const_iterator cend() const { return const_iterator{result_.pimpl_, Size()}; }
  const_iterator end() const { return cend(); }
  //@}
  //@{
  /** @name Reverse iteration */
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(const_iterator(result_.pimpl_, Size() - 1));
  }
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(const_iterator(result_.pimpl_, npos));
  }
  const_reverse_iterator rend() const { return crend(); }
  //@}
  //@{
  /** @name Random access interface */
  /// Access a row by index
  /// Accessing a row beyond the result set size is undefined behaviour
  reference operator[](size_type index) const {
    return UserData::FromRow(result_[index]);
  }
  /// Range-checked access to a row by index
  /// Accessing a row beyond the result set size will throw an exception
  /// @throws RowIndexOutOfBounds
  reference At(size_type index) const {
    return UserData::FromRow(result_.At(index));
  }
  //@}
  //@}
 private:
  ResultSet result_;
};

}  // namespace storages::postgres
