# Source Code Guidelines

## Code Formatting

Always observe the .editorconfig and .clang-format settings defined in the root of the repository.

# Unit Test Guidelines

## Code Formatting Standards

### Test Structure

All unit tests should follow the **Arrange-Act-Assert** pattern with clear visual separation between each section using blank lines.

#### Required Format

```cpp
TEST_F(TestClassName, testMethodName)
{
    // Arrange - Setup test data and expectations
    const auto input = createTestInput();
    const auto expected = "expected_result";
  
    // Act - Execute the code under test
    const auto result = methodUnderTest(input);
    
    // Assert - Verify the results
    EXPECT_EQ(expected, result);
}
```

#### Parameterized Tests

For parameterized tests, the same structure applies:

```cpp
TEST_P(ParameterizedTestClass, testMethodName)
{
    // Arrange
    const auto& param = GetParam();
    const auto input = param.input;
  
    // Act
    const auto result = methodUnderTest(input);
    
    // Assert
    EXPECT_EQ(param.expected, result);
}
```

### Key Requirements

1. **Blank Line Separation**: Always include blank lines between:
   - Setup (Arrange) section
- Execution (Act) section  
   - Verification (Assert) section

2. **Clear Comments**: Use comments to identify each section when the separation isn't obvious:
   - `// Arrange` - Test setup and input preparation
   - `// Act` - Execution of the method under test
   - `// Assert` - Verification of results and expectations

3. **Single Responsibility**: Each test should verify one specific behavior or scenario

4. **Descriptive Names**: Test names should clearly describe what is being tested

### Examples

#### ? Good Example
```cpp
TEST_F(TestSimplifyBinaryOp, addTwoNumbers)
{
    // Arrange
 const Expr expression = binary(number(7.0), '+', number(12.0));
 
    // Act
    const Expr simplified = simplify(expression);
    
    // Assert
  simplified->visit(formatter);
    EXPECT_EQ("number:19\n", str.str());
}
```

#### ? Bad Example
```cpp
TEST_F(TestSimplifyBinaryOp, addTwoNumbers)
{
    const Expr expression = binary(number(7.0), '+', number(12.0));
    const Expr simplified = simplify(expression);
    simplified->visit(formatter);
    EXPECT_EQ("number:19\n", str.str());
}
```

### Framework-Specific Guidelines

#### Google Test (GTest)
- Use `TEST_F` for fixture-based tests
- Use `TEST_P` for parameterized tests
- Use descriptive assertion macros (`EXPECT_EQ`, `EXPECT_TRUE`, etc.)
- Include meaningful failure messages when appropriate

### Benefits

Following this structure provides:
- **Improved Readability**: Clear separation makes tests easier to understand
- **Better Maintainability**: Changes to one section don't affect others
- **Easier Debugging**: Failed tests are easier to analyze
- **Consistent Style**: Uniform formatting across the codebase

### Enforcement

These guidelines should be:
- Reviewed during code reviews
- Enforced by automated formatting tools where possible
- Applied to both new tests and refactored existing tests
