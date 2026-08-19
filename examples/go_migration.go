// ============================================================================
// go_migration_demo.go — Go code for migration testing
// ============================================================================
//
// Demonstrates Go patterns that the migration tool can parse and translate
// to Novium intermediate representation.
//
// ============================================================================

// Package declaration
package main

// Import standard library
import (
	"fmt"
	"math"
)

// Structure definition
type Point struct {
	X float64
	Y float64
}

// Function with named return
func add(x int, y int) (result int) {
	result = x + y
	return
}

// Method on struct
func (p *Point) distance() float64 {
	return math.Sqrt(p.X*p.X + p.Y*p.Y)
}

// Variadic function
func sum(nums ...int) int {
	total := 0
	for _, n := range nums {
		total += n
	}
	return total
}

// Higher-order function
func apply(f func(int) int, values []int) []int {
	result := make([]int, len(values))
	for i, v := range values {
		result[i] = f(v)
	}
	return result
}

// Main function
func main() {
	// Basic usage
	a := add(5, 3)
	fmt.Printf("add(5, 3) = %d\n", a)

	// Point creation and method call
	p := Point{X: 3.0, Y: 4.0}
	d := p.distance()
	fmt.Printf("distance = %f\n", d)

	// Variadic function
	nums := []int{1, 2, 3, 4, 5}
	total := sum(nums...)
	fmt.Printf("sum = %d\n", total)

	// Higher-order function
	doubleFn := func(v int) int {
		return v * 2
	}
	doubled := apply(doubleFn, nums)
	fmt.Printf("doubled = %v\n", doubled)
}