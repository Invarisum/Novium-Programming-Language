// ============================================================================
// Error Correction Module - Main entry point for the error correction feature
// ============================================================================

import { ErrorCorrector } from './error-corrector';
import { HardwareDetector, HardwareProfile } from '../src/hardware-detector';
import { DiagnosticIssue } from '../src/diagnostic-engine';

export interface CorrectAndCompileOptions {
  sourceCode: string;
  target: 'native' | 'wasm' | 'cuda' | 'gpu';
  projectRoot: string;
}

export interface CorrectAndCompileResult {
  success: boolean;
  fixedCode: string;
  bugsFixed: DiagnosticIssue[];
  bugsEncountered: DiagnosticIssue[];
  compilationErrors: string[];
  summary: string;
}

export class ErrorCorrectionModule {
  private corrector: ErrorCorrector;

  constructor() {
    this.corrector = new ErrorCorrector();
  }

  // Main entry point: correct errors and compile
  async correctAndCompile(options: CorrectAndCompileOptions): Promise<CorrectAndCompileResult> {
    const { sourceCode, target, projectRoot } = options;

    // Run error correction and compilation
    const fixResult = await this.corrector.correctAndCompile(sourceCode, target, projectRoot);

    // Extract bugs fixed and encountered
    const bugsFixed: DiagnosticIssue[] = fixResult.fixedIssues;
    const allEncountered = [
      ...bugsFixed,
      ...fixResult.unfixableIssues,
      ...fixResult.unchangedIssues
    ];

    // Generate summary
    const summary = this.corrector.generateFixSummary(fixResult);

    return {
      success: fixResult.success,
      fixedCode: fixResult.fixedCode,
      bugsFixed,
      bugsEncountered: allEncountered,
      compilationErrors: fixResult.compilationErrors,
      summary
    };
  }

  // Convenience method for terminal usage
  async runTerminalCompile(
    sourceCode: string,
    target: string,
    projectRoot: string
  ): Promise<void> {
    // Map string target to enum
    const targetMap: Record<string, 'native' | 'wasm' | 'cuda' | 'gpu'> = {
      'native': 'native',
      'wasm': 'wasm',
      'cuda': 'cuda',
      'gpu': 'gpu'
    };

    const mappedTarget = targetMap[target] || 'native';

    console.log('[Error Correction] Starting error correction and compilation...');
    console.log('');

    const result = await this.correctAndCompile({
      sourceCode,
      target: mappedTarget,
      projectRoot
    });

    // Display results
    console.log(result.summary);
    console.log('');

    if (result.success) {
      console.log('✅ Compilation successful! No critical errors remaining.');
      console.log('');
      console.log('Fixed bugs:');
      result.bugsFixed.forEach((bug, i) => {
        console.log(`  ${i + 1}. ${bug.message}`);
        if (bug.description) {
          console.log(`     ${bug.description}`);
        }
      });
    } else {
      console.log('❌ Compilation failed with remaining errors:');
      result.compilationErrors.forEach((err, i) => {
        console.log(`  ${i + 1}. ${err}`);
      });

      console.log('');
      console.log('Bugs encountered during correction:');
      result.bugsEncountered.forEach((bug, i) => {
        console.log(`  ${i + 1}. ${bug.message} [${bug.severity}]`);
        if (bug.description) {
          console.log(`     ${bug.description}`);
        }
      });

      console.log('');
      console.log('These bugs could not be automatically fixed and require manual attention.');
    }

    console.log('');
    console.log('--- Fixed Code ---');
    console.log(result.fixedCode);
  }
}

// CLI interface if run directly
if (require.main === module) {
  const module = require('.');
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.error('Usage: node error-correction.js <target> <file>');
    process.exit(1);
  }

  const target = args[0];
  const filePath = args[1];

  // Read source code from file
  const fs = require('fs');
  const sourceCode = fs.readFileSync(filePath, 'utf-8');

  // Determine project root (directory of the file)
  const projectRoot = require('path').dirname(filePath);

  // Run error correction and compilation
  module.ErrorCorrectionModule.prototype
    .runTerminalCompile(sourceCode, target, projectRoot)
    .then(() => {
      console.log('\n[Error Correction] Process complete.');
    })
    .catch((error: Error) => {
      console.error('[Error Correction] Fatal error:', error.message);
      process.exit(1);
    });
}
