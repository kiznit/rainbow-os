# Backward compatibility with earlier versions of C\+\+

- [Introduction](#introduction)
- [C\+\+11 compromises](#cxx11_compromises)
  - [C\+\+11 API](#cxx11_api)
    - [`REQUIRE_CALL` example](#cxx11_require_call)
    - [`NAMED_REQUIRE_CALL` example](#cxx11_named_require_call)
    - [`ALLOW_CALL` example](#cxx11_allow_call)
    - [`NAMED_ALLOW_CALL` example](#cxx11_named_allow_call)
    - [`FORBID_CALL` example](#cxx11_forbid_call)
    - [`NAMED_FORBID_CALL` example](#cxx11_named_forbid_call)
    - [Migrating from C\+\+11 API to C\+\+14 API](#migrate_to_cxx14)
  - [Implicit conversion of `RETURN` modifier expression](#cxx11_return_implicit_conversion)
  - [Macro expansion of `ANY` matcher before stringizing](#cxx11_any_stringizing)
- [G\+\+ 4.8.x limitations](#gxx48x_limitations)
  - [Compiler defects](#gxx48x_compiler)
    - [Rvalue reference failures](gxx48x_compiler_rvalue_ref)
    - [Overload failures](#gxx48x_compiler_overload)
    - [! matcher failures](#gxx48x_compiler_neg_matcher)
    - [ANY matcher failures](#gxx48x_compiler_any_matcher)
    - [Summary of compiler matcher failures](#gxx48x_compiler_matcher_summary)
  - [Standard library defects](#gxx48x_library)
    - [Regular expressions](#gxx48x_library_regex)
    - [`<tuple>`](#gxx48x_library_tuple)

## <A name="introduction"/> Introduction

Trompeloeil is a C\+\+14-first library and looks forward to the future
where C\+\+17-and-beyond language features and standard library
further improve its utility and performance.

Sometimes though, a project comes along where instead of looking forward
one has to look backward.  How far backward?  All the way back to
`g++-4.8.4` with a vintage `libstdc++-v3` library and requirements for
compliance with the features available in the C\+\+11 standard without
compiler extensions.  (Indeed, it might be much worse: you might have
a project forced to use `g++-4.8.3` with further constraints.)

To allow you to consider Trompeloeil as your mock object framework
for a project with these constraints, we have back ported Trompeloeil to
`g++-4.8.4` (C\+\+11, `libstdc++-v3`) with an API that may be used with
a C\+\+11 dialect selected.  The C\+\+11 API remains supported when a C\+\+14
(or later) dialect is selected.

## <A name="cxx11_compromises"/> C\+\+11 compromises

Two major changes to functionality occur when compiling with the C\+\+11 dialect.

First, the API for defining expectations is different from the API used in
C\+\+14 and later dialects.

Second, the argument to a [**`RETURN`**](reference.md/#RETURN) modifier
undergoes an implicit conversion before semantic analysis.  This results in
a different error message when the expression cannot be implicitly converted
to the return type of the function for which the expectation is being defined.

A minor change is that the expansion of the [**`ANY`**](reference.md/#ANY)
matcher in arguments to expectations is stringized differently in the
C\+\+11 API when compared to the C\+\+14 API.

### <A name="cxx11_api"/> C\+\+11 API

It has not been possible to replicate the macro syntax that is used in
the C\+\+14 API.  The cause is an inability to find a mechanism to
communicate sufficient type information between expectation macros and
modifier macros.

Our approach has been to redefine expectation macros as taking the
modifiers, if any, as an argument to the expectation macro.

The C\+\+11 API has a similar name to the corresponding C\+\+14 API

```text
C++14 API                       C++11 API
------------------------------  --------------------------------
TROMPELOEIL_REQUIRE_CALL        TROMPELOEIL_REQUIRE_CALL_V
TROMPELOEIL_NAMED_REQUIRE_CALL  TROMPELOEIL_NAMED_REQUIRE_CALL_V
TROMPELOEIL_ALLOW_CALL          TROMPELOEIL_ALLOW_CALL_V
TROMPELOEIL_NAMED_ALLOW_CALL    TROMPELOEIL_NAMED_ALLOW_CALL_V
TROMPELOEIL_FORBID_CALL         TROMPELOEIL_FORBID_CALL_V
TROMPELOEIL_NAMED_FORBID_CALL   TROMPELOEIL_NAMED_FORBID_CALL_V
```

Short names for these macros have also been defined.

```text
Short macro name            Long macro name
--------------------        --------------------------------
REQUIRE_CALL_V              TROMPELOEIL_REQUIRE_CALL_V
NAMED_REQUIRE_CALL_V        TROMPELOEIL_NAMED_REQUIRE_CALL_V
ALLOW_CALL_V                TROMPELOEIL_ALLOW_CALL_V
NAMED_ALLOW_CALL_V          TROMPELOEIL_NAMED_ALLOW_CALL_V
FORBID_CALL_V               TROMPELOEIL_FORBID_CALL_V
NAMED_FORBID_CALL_V         TROMPELOEIL_NAMED_FORBID_CALL_V
```

In the C\+\+11 API, the expectation macros accept two or more
arguments, the first two being object and function and the remainder
being the modifiers, if any.  Therefore the `_V` suffix
may be read as an abbreviation of the word "variadic".

All documentation in Trompeloeil outside of this note uses the C\+\+14 API.
The examples below show how to translate from the C\+\+14 API to the C\+\+11
API.

#### <A name="cxx11_require_call"/> `REQUIRE_CALL` example

Given mock class `mock_c` with mock function `int getter(int)`,

```cpp
struct mock_c
{
  MAKE_MOCK1(getter, int(int));
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  // Two macros adjacent to each other in the program text.
  REQUIRE_CALL(obj, getter(ANY(int)))
    .RETURN(_1);
```

the corresponding expectation implemented using the C\+\+11 API is

```cpp
  mock_c obj;

  // One macro with three arguments.
  REQUIRE_CALL_V(obj, getter(ANY(int)),
    .RETURN(_1));
```

See also: [**`REQUIRE_CALL(...)`**](reference.md/#REQUIRE_CALL).

#### <A name="cxx11_named_require_call"/> `NAMED_REQUIRE_CALL` example

Given mock class `mock_c` with mock function `int getter(int)`,

```cpp
struct mock_c
{
  MAKE_MOCK1(getter, int(int));
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  // Two macros adjacent to each other in the program text.
  auto e = NAMED_REQUIRE_CALL(obj, getter(ANY(int)))
    .RETURN(0);
```

the corresponding expectation implemented using the C\+\+11 API is

```cpp
  mock_c obj;

  // One macro with three arguments.
  auto e = NAMED_REQUIRE_CALL_V(obj, getter(ANY(int)),
    .RETURN(0));
```

See also: [**`NAMED_REQUIRE_CALL(...)`**](reference.md/#NAMED_REQUIRE_CALL).

#### <A name="cxx11_allow_call"/> `ALLOW_CALL` example

Given mock class `mock_c` with mock function `int count()`,

```cpp
struct mock_c
{
  MAKE_MOCK0(count, int());
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  // Two macros adjacent to each other in the program text.
  ALLOW_CALL(obj, count())
    .RETURN(1);
```

the corresponding expectation implemented using the C\+\+11 API is,

```cpp
  mock_c obj;

  // One macro with three arguments.
  ALLOW_CALL_V(obj, count(),
    .RETURN(1));
```

See also: [**`ALLOW_CALL(...)`**](reference.md/#ALLOW_CALL).

#### <A name="cxx11_named_allow_call"/> `NAMED_ALLOW_CALL` example

Given mock class `mock_c` with mock function `int count()`,

```cpp
struct mock_c
{
  MAKE_MOCK0(count, int());
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  // Two macros adjacent to each other in the program text.
  auto e = NAMED_ALLOW_CALL(obj, count())
    .RETURN(1);
```

the corresponding expectation implemented using the C\+\+11 API is,

```cpp
  mock_c obj;

  // One macro with three arguments.
  auto e = NAMED_ALLOW_CALL_V(obj, count(),
    .RETURN(1));
```

See also: [**`NAMED_ALLOW_CALL(...)`**](reference.md/#NAMED_ALLOW_CALL).

#### <A name="cxx11_forbid_call"/> `FORBID_CALL` example

Given mock class `mock_c` with mock function `int count()`,

```cpp
struct mock_c
{
  MAKE_MOCK0(count, int());
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  FORBID_CALL(obj, count());
```

the corresponding expectation implemented using the C\+\+11 API is,

```cpp
  mock_c obj;

  FORBID_CALL_V(obj, count());
```

See also: [**`FORBID_CALL(...)`**](reference.md/#FORBID_CALL).

#### <A name="cxx11_named_forbid_call"/> `NAMED_FORBID_CALL` example

Given mock class `mock_c` with mock function `int count()`,

```cpp
struct mock_c
{
  MAKE_MOCK0(count, int());
};
```

and this expectation defined using the C\+\+14 API,

```cpp
  mock_c obj;

  auto e = NAMED_FORBID_CALL(obj, count());
```

the corresponding expectation implemented using the C\+\+11 API is,

```cpp
  mock_c obj;

  auto e = NAMED_FORBID_CALL_V(obj, count());
```

See also: [**`NAMED_FORBID_CALL(...)`**](reference.md/#NAMED_FORBID_CALL).

#### <A name="migrate_to_cxx14"/> Migrating from C\+\+11 API to C\+\+14 API

The C\+\+11 API remains available when changing your C\+\+ dialect to C\+\+14
or later.  Therefore existing test cases may be migrated incrementally
to the C\+\+14 API while using the C\+\+14 API for new test cases.

Steps:

- Remove the `_V` suffix from the expectation macro.
- Move the modifiers, if any, outside the argument list of the
  expectation macro.
- Remove workarounds such as
  - Changes to regular expressions that may be testing messages generated
    with the `ANY` matcher.
  - Any of the workarounds required to use Trompeloeil with `g++ 4.8.3`.

For example, given this expectation using the C\+\+11 API,

```cpp
  trompeloeil::sequence seq;

  auto thrown = false;

  REQUIRE_CALL_V(obj, getter(ANY(int)),
    .IN_SEQUENCE(seq)
    .LR_SIDE_EFFECT(thrown = true)
    .THROW(_1));
```

the corresponding C\+\+14 API implementation is,

```cpp
  trompeloeil::sequence seq;

  auto thrown = false;

  REQUIRE_CALL(obj, getter(ANY(int)))
    .IN_SEQUENCE(seq)
    .LR_SIDE_EFFECT(thrown = true)
    .THROW(_1);
```

### <A name="cxx11_return_implicit_conversion"/> Implicit conversion of `RETURN` modifier expression

In the C\+\+14 API, the [**`RETURN(...)`**](reference.md/#RETURN)
modifier stores the expression passed as an argument to the
modifier and forwards it to the Trompeloeil implementation
for semantic analysis.

In the C\+\+11 API, not only is the expression stored but also an
implicit conversion to the return type of the mocked function
is performed.  This implicit conversion may fail before
semantic checks are performed.

The result is that instead of a helpful message generated by
a `static_assert`, a less helpful error message is generated
by the compiler.

For example,

```cpp
struct MS
{
  MAKE_MOCK0(f, char&());
};

int main()
{
  MS obj;

  REQUIRE_CALL_V(obj, f(),
    .RETURN('a'));
}
```

The C\+\+11 error message is,

```text
error: invalid initialization of non-const reference of type
‘MS::trompeloeil_l_tag_type_trompeloeil_20::return_of_t {aka char&}’
from an rvalue of type ‘char’
```

The C\+\+14 error message is,

```text
error: static assertion failed:
RETURN non-reference from function returning reference
```

Other error messages that may not be generated while compiling using
a C\+\+11 dialect include:

```text
RETURN const* from function returning pointer to non-const

RETURN const& from function returning non-const reference

RETURN value is not convertible to the return type of the function
```

This is a quality of implementation issue that wasn't resolved before
the C\+\+11 API became available.

### <A name="cxx11_any_stringizing"/> Macro expansion of `ANY` matcher before stringizing

Macro expansion differences between C\+\+11 and C\+\+14 expectation macros
mean macros like the [**`ANY(...)`**](reference.md/#ANY_MACRO) matcher
used in the parameter list of the function to match are expanded in
C\+\+11 before they are stringized.

For example, a regular expression written in a C\+\+14 dialect, or later,

```cpp
  auto re = R":(Sequence expectations not met at destruction of sequence object "s":
  missing obj\.getter\(ANY\(int\)\) at [A-Za-z0-9_ ./:\]*:[0-9]*.*
  missing obj\.foo\(_\) at [A-Za-z0-9_ ./:\]*:[0-9]*.*
):";
```

won't match the string generated when running in a C\+\+11 dialect.
The `ANY(int)` matcher is actually expanded to some Trompeloeil-internal data.

A helper macro, such as `CXX11_AS_STRING()`, may be defined to give a
stringized form correct for C\+\+11:

```cpp
#define CXX11_AS_STRING_IMPL(x) #x
#define CXX11_AS_STRING(x) CXX11_AS_STRING_IMPL(x)
```

It may be used in constructing larger strings.  Rewriting the example above
illustrates its use:

```cpp
  auto re = R":(Sequence expectations not met at destruction of sequence object "s":
  missing obj\.getter\():" +
  escape_parens(CXX11_AS_STRING(ANY(int))) +
  R":(\) at [A-Za-z0-9_ ./:\]*:[0-9]*.*
  missing obj\.foo\(_\) at [A-Za-z0-9_ ./:\]*:[0-9]*.*
):";
```

where `escape_parens()` performs escaping of parentheses in the input string,

```cpp
  std::string escape_parens(const std::string& s)
  {
    constexpr auto backslash = '\\';

    std::string tmp;

    for (auto& c : s)
    {
      if (c == '(' || c == ')')
      {
        tmp += backslash;
      }

      tmp += c;
    }

    return tmp;
  }
```

This is a quality of implementation issue that wasn't resolved before
the C\+\+11 API became available.

## <A name="gxx48x_limitations"/> G\+\+ 4.8.x limitations

Trompeloeil's support for `g++ 4.8.x` is limited by compiler and standard
library defects.

### <A name="gxx48x_compiler"/> Compiler defects

`g++-4.8.3` and older suffers from [bug 58714](
https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58714) which causes the
wrong overload to be selected when overloads for mock functions exists
for both `T&&` and `const T&` parameters.

Example:

```Cpp
struct S
{
    MAKE_MOCK1(func, void(const int&));
    MAKE_MOCK1(func, void(int&&));
};

void test()
{
  S s;
  REQUIRE_CALL_V(s, func(ANY(const int&)));

  const int i = 0;
  s.func(i);
}
```

With `g++-4.8.3` and older, this is reported as:

```text
No match for call of func with signature void(const int&) with.

  param  _1 == 1
```

because the expectation is wrongly placed on the `int&&` overload.

Consider moving to a more modern compiler.

### <A name="gxx48x_library"/> Standard library defects

#### <A name="gxx48x_library_regex"/> Regular expressions

`g++ 4.8.x` lacks support for regular expressions in `libstdc++-v3`.

As Jonathan Wakely said,
> *sigh* \<regex\> is not implemented prior to GCC 4.9.0,
> I thought the whole world was aware of that by now.

See: GCC Bugzilla, "Bug 61582 - C\+\+11 regex memory corruption,"
23 June 2014. \
Available: <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61582> \
Accessed: 9 November 2017

As a consequence, avoid the [**`re(...)`**](reference.md/#re) matcher
in test cases when compiling with a C\+\+11 dialect.

#### <A name="gxx48x_library_tuple"/> `<tuple>`

`g++-4.8.3` has a defect in `<tuple>` in `libstdc++-v3`.

See: GCC Bugzilla, "Bug 61947 - [4.8/4.9 Regression] Ambiguous calls when constructing std::tuple,"
29 July 2014. \
Available: <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61947> \
Accessed: 8 November 2017

This is fixed in `g++-4.8.4`, `g++-4.9.3` and later releases.

One workaround for this defect is to copy a version of `<tuple>` from a
release of `g++-4.8.4` or `g++-4.8.5` and arrange to include it
ahead of the buggy version.  Then work hard to move forward to a more recent
compiler release.
