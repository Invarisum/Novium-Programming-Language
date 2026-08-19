// ============================================================================
// Test the Error Correction Module
// ============================================================================

import { ErrorCorrectionModule } from './error-correction/index';

// Read the test file
const fs = require('fs');
const path = require('path');

const testFile = path.join(__dirname, 'test-code.nvm');
const sourceCode = fs.readFileSync(testFile, 'utf-8');

const module = new ErrorCorrectionModule();

// Test with different targets
const targets = ['native', 'wasm', 'cuda', 'gpu'];

(async () => {
  console.log('=== Novium Error Correction Test ===');
  console.log('');

  for (const target of targets) {
    console.log(`--- Target: ${target} ---`);
    console.log('');

    const result = await module.correctAndCompile({
      sourceCode,
      target: target as any,
      projectRoot: __dirname
    });

    console.log(result.summary);
    console.log('');
  }
})().catch((err: Error) => {
  console.error('Test failed:', err.message);
});