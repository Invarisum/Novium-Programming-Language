//go:build !repl

// ============================================================================
// novium_cli_demo.go �?" Novium CLI Demo (Go)
// Demonstrates CLI patterns for the Novium language toolchain.
// ============================================================================

package main

import "os"

// ── Available Commands ──────────────────────────────────────────────────────

// help — Show this help message
func CmdHelp() string {
	return `Novium CLI v0.2

Usage:
  novium <command> [file.nvm]

Commands:
  help       Show this help message
  run        Execute the Novium v0.1 core subset
  check      Parse and type-check without execution
  ast        Print parsed Abstract Syntax Tree
  tokens     Print lexer tokens only
  codegen    Generate LLVM IR code
  build      Build/compile a Novium program

Examples:
  novium examples/hello.nvm
  novium --tokens examples/hello.nvm
  novium --check examples/hello.nvm`
}

// run — Execute a Novium program
func CmdRun(filepath string) error {
	// In a full implementation, this would invoke the Novium runner
	// For now, just indicate the command
	println("Running:", filepath)
	return nil
}

// check — Parse and type-check a Novium program
func CmdCheck(filepath string) error {
	println("Type-checking:", filepath)
	return nil
}

// ast — Print the parsed Abstract Syntax Tree
func CmdAst(filepath string) error {
	println("Printing AST for:", filepath)
	return nil
}

// tokens — Print lexer tokens only
func CmdTokens(filepath string) error {
	println("Printing tokens for:", filepath)
	return nil
}

// codegen — Generate LLVM IR code
func CmdCodegen(filepath string) error {
	println("Generating LLVM IR for:", filepath)
	return nil
}

// build — Build/compile a Novium program
func CmdBuild(filepath string, target string) error {
	println("Building", filepath, "for target:", target)
	return nil
}

// package — Package manager client
func CmdPackage(args []string) error {
	switch {
	case len(args) > 0 && args[0] == "install":
		if len(args) > 1 {
			println("Installing package:", args[1])
		} else {
			println("Usage: novium pkg install <package_name>")
		}
	case len(args) > 0 && args[0] == "list":
		println("Listing installed packages...")
		println("  core - Core Novium library")
		println("  stdlib - Standard library")
	case len(args) > 0 && args[0] == "search":
		if len(args) > 1 {
			println("Searching for:", args[1])
		} else {
			println("Usage: novium pkg search <query>")
		}
	default:
		println("Novium Package Manager")
		println("Usage:")
		println("  novium pkg install <package>   - Install a package")
		println("  novium pkg list              - List installed packages")
		println("  novium pkg search <query>    - Search for packages")
	}
	return nil
}

// ide — IDE integration hints
func CmdIde(filepath string) error {
	println("IDE Integration for:", filepath)
	println("  - Syntax highlighting for .nvm, .nvi, .nvw files")
	println("  - Type diagnostics and error detection")
	println("  - Code completion suggestions")
	println("  - Go to definition and rename refactoring")
	println("  - Build integration: novium build --target <target>")
	println("  - Test runner integration")
	return nil
}

// main — CLI Entry Point
func main() {
	if len(os.Args) < 2 {
		print(CmdHelp())
		return
	}

	command := os.Args[1]
	filepath := ""
	var args []string

	// Parse remaining arguments
	if len(os.Args) > 2 {
		filepath = os.Args[2]
		args = os.Args[3:]
	}

	switch command {
	case "help":
		print(CmdHelp())
	case "run":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdRun(filepath); err != nil {
			print("Error running:", err.Error())
		}
	case "check":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdCheck(filepath); err != nil {
			print("Error checking:", err.Error())
		}
	case "ast":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdAst(filepath); err != nil {
			print("Error printing AST:", err.Error())
		}
	case "tokens":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdTokens(filepath); err != nil {
			print("Error printing tokens:", err.Error())
		}
	case "codegen":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdCodegen(filepath); err != nil {
			print("Error generating code:", err.Error())
		}
	case "build":
		target := "native"
		if len(args) > 0 {
			target = args[0]
		}
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdBuild(filepath, target); err != nil {
			print("Error building:", err.Error())
		}
	case "pkg":
		if err := CmdPackage(args); err != nil {
			print("Error package operation:", err.Error())
		}
	case "ide":
		if filepath == "" {
			print("Error: No file specified")
			print(CmdHelp())
			return
		}
		if err := CmdIde(filepath); err != nil {
			print("Error IDE operation:", err.Error())
		}
	default:
		print("Unknown command: ", command)
		print(CmdHelp())
	}
}