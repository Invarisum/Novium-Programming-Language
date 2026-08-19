# ============================================================================
# python_migration_demo.py — Python code for migration testing
#
# Demonstrates Python patterns that the migration tool can parse and translate
# to Novium intermediate representation.
# ============================================================================

# Mathematical constant
PI = 3.141592653589793

E = 2.718281828459045


# Function with type hints (Python 3.x)
def absolute_value(x: int) -> int:
    """Return the absolute value of x."""
    if x < 0:
        return -x
    return x


# Function with default arguments
def max_val(a: int, b: int = 0) -> int:
    """Return the maximum of a and b."""
    if a > b:
        return a
    return b


# Class with methods
class Point:
    """A point in 2D space."""

    def __init__(self, x: float = 0.0, y: float = 0.0):
        self.x = x
        self.y = y

    def distance(self) -> float:
        """Calculate the distance from the origin."""
        return (self.x ** 2 + self.y ** 2) ** 0.5


# Higher-order function
def apply_func(f, values):
    """Apply function f to each value in the list."""
    result = []
    for v in values:
        result.append(f(v))
    return result


# Recursive function (factorial)
def factorial(n: int) -> int:
    """Return n! (factorial of n)."""
    if n <= 1:
        return 1
    return n * factorial(n - 1)


# Main execution
if __name__ == "__main__":
    # Basic usage
    x: int = absolute_value(-42)
    print(f"absolute_value(-42) = {x}")

    # Max value
    m: int = max_val(10, 20)
    print(f"max_val(10, 20) = {m}")

    # Point class
    p = Point(x=3.0, y=4.0)
    d: float = p.distance()
    print(f"Point(3, 4).distance() = {d}")

    # Apply function
    nums: list = [1, 2, 3, 4, 5]
    doubled: list = apply_func(lambda v: v * 2, nums)
    print(f"doubled = {doubled}")

    # Factorial
    n: int = 5
    f: int = factorial(n)
    print(f"factorial(5) = {f}")