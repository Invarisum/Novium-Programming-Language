// ============================================================================
// pm_backend.go — Novium Package Manager Backend
// ============================================================================
//
// Handles package lifecycle: install, publish, search, list.
// Supports local package registry and remote registry integration.
//
// ============================================================================

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

// ── Package Metadata ───────────────────────────────────────────────────────

// PackageMeta describes a Novium package
type PackageMeta struct {
	Name    string `json:"name"`
	Version string `json:"version"`
	Desc    string `json:"desc,omitempty"`
	// Main       string `json:"main,omitempty"`    // Entry point .nvm file
	// Author     string `json:"author,omitempty"`
	// License    string `json:"license,omitempty"`
	// Tags       []string `json:"tags,omitempty"`
	// DependsOn  []string `json:"depends_on,omitempty"`
	// Repo       string `json:"repo,omitempty"` // Source repository URL
}

// InstalledPackage extends PackageMeta with installation path
type InstalledPackage struct {
	PackageMeta
	Path string `json:"-"` // Installation directory
}

// ── Package Manager State ──────────────────────────────────────────────────

// PkgState tracks the package manager's state
type PkgState struct {
	// Installed packages (name -> package)
	Installed map[string]*InstalledPackage
	// Cache of remote registry packages
	RegistryCache map[string]*PackageMeta
	// Current registry URL
	RegistryURL string
	// Home directory for Novium data
	NoviumHome string
}

// NewPkgState creates a new package state
func NewPkgState(noviumHome string) *PkgState {
	return &PkgState{
		Installed:     make(map[string]*InstalledPackage),
		RegistryCache: make(map[string]*PackageMeta),
		RegistryURL:   "https://registry.novium-lang.org",
		NoviumHome:    noviumHome,
	}
}

// ── Package Discovery ──────────────────────────────────────────────────────

// ListInstalled returns all installed packages
func (ps *PkgState) ListInstalled() []*InstalledPackage {
	var result []*InstalledPackage
	for _, pkg := range ps.Installed {
		result = append(result, pkg)
	}
	return result
}

// SearchRegistry searches the remote registry for packages matching query
func (ps *PkgState) SearchRegistry(query string) ([]*PackageMeta, error) {
	// Check cache first
	if cached, ok := ps.RegistryCache[query]; ok {
		return []*PackageMeta{cached}, nil
	}

	// In a full implementation, this would make an HTTP request
	// For now, simulate with known packages
	var results []*PackageMeta

	// Simulated packages
	simulated := []PackageMeta{
		{Name: "core", Version: "0.1.0", Desc: "Core Novium library"},
		{Name: "stdlib", Version: "0.1.0", Desc: "Standard library"},
		{Name: "web", Version: "0.1.0", Desc: "Web framework components"},
		{Name: "gui", Version: "0.1.0", Desc: "Graphical UI toolkit"},
		{Name: "db", Version: "0.1.0", Desc: "Database drivers"},
	}

	for _, pkg := range simulated {
		if strings.Contains(strings.ToLower(pkg.Name), strings.ToLower(query)) ||
			strings.Contains(strings.ToLower(pkg.Desc), strings.ToLower(query)) {
			results = append(results, &pkg)
		}
	}

	// Cache the query result
	if len(results) > 0 {
		ps.RegistryCache[query] = results[0] // Cache first result
	}

	return results, nil
}

// ── Package Installation ───────────────────────────────────────────────────

// InstallPackage installs a package from the registry
func (ps *PkgState) InstallPackage(name, version string) error {
	// Check if already installed
	if _, exists := ps.Installed[name]; exists {
		return fmt.Errorf("package %s already installed", name)
	}

	// In a full implementation, this would:
	// 1. Download the package from the registry
	// 2. Verify checksum/signature
	// 3. Extract to the Novium packages directory
	// 4. Update the symbol table with exported functions/types

	// For now, simulate installation
	pkgPath := filepath.Join(ps.NoviumHome, "packages", name+"-"+version)
	if err := os.MkdirAll(pkgPath, 0755); err != nil {
		return err
	}

	// Create a placeholder package metadata file
	meta := &InstalledPackage{
		PackageMeta: PackageMeta{
			Name:    name,
			Version: version,
			Desc:    "Installed from registry",
		},
		Path: pkgPath,
	}

	ps.Installed[name] = meta
	return nil
}

// UninstallPackage removes an installed package
func (ps *PkgState) UninstallPackage(name string) error {
	if _, exists := ps.Installed[name]; !exists {
		return fmt.Errorf("package %s not installed", name)
	}

	pkg := ps.Installed[name]
	pkgPath := pkg.Path

	// Remove the package directory
	if err := os.RemoveAll(pkgPath); err != nil {
		return err
	}

	// Remove from installed list
	delete(ps.Installed, name)
	return nil
}

// ── Package Publishing ──────────────────────────────────────────────────────

// PublishPackage publishes a local package to the registry
func (ps *PkgState) PublishPackage(name, version, desc string) error {
	// In a full implementation, this would:
	// 1. Validate the package structure
	// 2. Upload to the remote registry
	// 3. Update the package index
	// 4. Set the version and visibility

	// For now, just simulate
	meta := &PackageMeta{
		Name:    name,
		Version: version,
		Desc:    desc,
	}
	ps.RegistryCache[name] = meta
	return nil
}

// ── Command Execution ──────────────────────────────────────────────────────

// ExecuteCmd runs a package manager command
func (ps *PkgState) ExecuteCmd(cmd string, args []string) error {
	switch cmd {
	case "install":
		if len(args) < 1 {
			return fmt.Errorf("Usage: novium pkg install <package>[@<version>]")
		}
		// Parse package@version
		packageSpec := args[0]
		packageName := packageSpec
		packageVersion := "latest"
		if idx := strings.Index(packageSpec, "@"); idx >= 0 {
			packageName = packageSpec[:idx]
			packageVersion = packageSpec[idx+1:]
		}
		if err := ps.InstallPackage(packageName, packageVersion); err != nil {
			return err
		}
		fmt.Printf("Installed package: %s@%s\n", packageName, packageVersion)
		return nil

	case "list":
		pkgs := ps.ListInstalled()
		if len(pkgs) == 0 {
			fmt.Println("No packages installed.")
			return nil
		}
		fmt.Println("Installed packages:")
		for _, pkg := range pkgs {
			fmt.Printf("  %s@%s - %s\n", pkg.Name, pkg.Version, pkg.Desc)
		}
		return nil

	case "search":
		if len(args) < 1 {
			return fmt.Errorf("Usage: novium pkg search <query>")
		}
		results, err := ps.SearchRegistry(args[0])
		if err != nil {
			return err
		}
		if len(results) == 0 {
			fmt.Printf("No packages found matching '%s'\n", args[0])
			return nil
		}
		fmt.Printf("Packages matching '%s':\n", args[0])
		for _, pkg := range results {
			fmt.Printf("  %s@%s - %s\n", pkg.Name, pkg.Version, pkg.Desc)
		}
		return nil

	case "publish":
		if len(args) < 3 {
			return fmt.Errorf("Usage: novium pkg publish <name> <version> <description>")
		}
		if err := ps.PublishPackage(args[0], args[1], args[2]); err != nil {
			return err
		}
		fmt.Printf("Published package: %s@%s\n", args[0], args[1])
		return nil

	case "info":
		if len(args) < 1 {
			return fmt.Errorf("Usage: novium pkg info <package>")
		}
		if pkg, ok := ps.Installed[args[0]]; ok {
			fmt.Printf("Package: %s@%s\n", pkg.Name, pkg.Version)
			fmt.Printf("Description: %s\n", pkg.Desc)
			fmt.Printf("Location: %s\n", pkg.Path)
		} else {
			fmt.Printf("Package '%s' not found locally.\n", args[0])
			// Try registry search
			results, _ := ps.SearchRegistry(args[0])
			if len(results) > 0 {
				fmt.Printf("Available in registry: %s@%s - %s\n", results[0].Name, results[0].Version, results[0].Desc)
			}
		}
		return nil

	default:
		return fmt.Errorf("Unknown package manager command: %s", cmd)
	}
}

// ── CLI Entry Point ────────────────────────────────────────────────────────

// main runs the package manager CLI
func main() {
	// Determine Novium home directory
	noviumHome := ""
	if home := os.Getenv("NOVIUM_HOME"); home != "" {
		noviumHome = home
	} else {
		// Default to ~/.novium on Unix, %APPDATA%\\novium on Windows
		// For now, use current directory companion
		noviumHome = "./.novium"
	}

	// Initialize package state
	ps := NewPkgState(noviumHome)

	// If no command, show help
	if len(os.Args) < 2 {
		fmt.Println("Novium Package Manager v0.2")
		fmt.Println("Usage:")
		fmt.Println("  novium pkg install <package>[@<version>]   - Install a package")
		fmt.Println("  novium pkg list                            - List installed packages")
		fmt.Println("  novium pkg search <query>                  - Search for packages")
		fmt.Println("  novium pkg publish <name> <version> <desc> - Publish a package")
		fmt.Println("  novium pkg info <package>                  - Show package info")
		fmt.Println("  novium pkg help                            - Show this help")
		os.Exit(0)
	}

	// Execute command
	cmd := os.Args[1]
	args := []string{}
	if len(os.Args) > 2 {
		args = os.Args[2:]
	}

	if err := ps.ExecuteCmd(cmd, args); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}
}