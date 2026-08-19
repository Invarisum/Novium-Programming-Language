# Error Correction Module

## Overview
The Error Correction feature automatically detects and fixes critical code errors during compilation. When you compile Novium code (.nvm, .nvi, .nvw files), the error corrector:

1. **Analyzes** the source code for common programming errors
2. **Automatically fixes** fixable issues (dependency conflicts, style problems, security patterns, build config issues, etc.)
3. **Compiles** the corrected code
4. **Reports** what bugs were fixed and what bugs remain

## How It Works

### 1. Diagnosis
- Runs a full diagnostic pass over the source code
- Identifies issues in categories: dependency, build, performance, security, style, hardware

### 2. Automatic Fixing
- Applies fix patterns based on issue type:
  - **Dependency**: Fix version conflicts, remove duplicate imports
  - **Build**: Add missing imports, fix type errors, configure compiler flags
  - **Performance**: Optimize loops, remove unnecessary computations
  - **Security**: Remove eval(), sanitize inputs, fix XSS patterns
  - **Style**: Format code, fix naming conventions
  - **Hardware**: Adjust target architecture, optimize for detected hardware

### 3. Compilation
- Compiles the corrected code with the selected target (native/wasm/cuda/gpu)
- Captures all compilation errors and warnings

### 4. Reporting
- Shows a summary of all fixes applied
- Lists bugs that were automatically fixed
- Lists bugs that required manual attention
- Shows remaining compilation errors
- Displays the final fixed code

## Usage

### Via TypeScript/Node.js
```typescript
import { ErrorCorrectionModule } from './error-correction';

const module = new ErrorCorrectionModule();

const result = await module.correctAndCompile({
  sourceCode: `fn main() void: print("Hello")`,
  target: 'native',
  projectRoot: './my-project'
});

console.log(result.summary);
console.log('Fixed code:', result.fixedCode);
```

### Via Terminal/CLI
```bash
# Using the built module
node error-correction.js native test-code.nvm
```

### Via VS Code Extension
The Novium Adaptive Extension includes the "Diagnose & Fix" command which runs the error corrector and shows results in a panel.

## Test File

The `test-code.nvm` file in this directory contains intentionally broken Novium code with various bug types:
- Unused variables
- Syntax errors (triple semicolons)
- Missing semicolons
- Redefinitions
- Misplaced braces
- Missing return types
- Infinite loop patterns

When you run the error corrector on this file, it will demonstrate fixing multiple issue types and report the results.

## Integration

The error corrector integrates with:
- **Hardware Detector** - Adapts fixes to your specific CPU/GPU hardware
- **Diagnostic Engine** - Provides issue analysis and auto-fix capabilities
- **Build Optimizer** - Ensures corrected code compiles with optimal settings
- **VS Code Extension** - Provides UI integration for the Novium Adaptive Toolchain

## Configuration

The error corrector can be configured via the Novium project's `novium.toml`:

```toml
[error_correction]
  autoFix = true        # Auto-apply fixes during compilation
  maxAutoFixAttempts = 3  # Maximum auto-fix attempts per compilation
  reportStyle = "detailed"  # Summary report style: "brief" or "detailed"
```