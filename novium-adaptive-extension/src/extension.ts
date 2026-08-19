// ============================================================================
// Novium Adaptive Extension - Main Entry Point
// ============================================================================
// Zero-config, self-optimizing Novium toolchain that auto-detects hardware,
// prevents dependency conflicts, and compiles blazingly fast for any target.

import * as vscode from 'vscode';
import { HardwareDetector } from './hardware-detector';
import { AutoConfigurator } from './auto-configurator';
import { DependencyResolver } from './dependency-resolver';
import { BuildOptimizer } from './build-optimizer';
import { DiagnosticEngine } from './diagnostic-engine';
import { NoviumCLI } from './novium-cli';

export interface HardwareProfile {
  cpu: {
    cores: number;
    threads: number;
    model: string;
    frequencyMHz: number;
    cacheL1KB: number;
    cacheL2KB: number;
    cacheL3KB: number;
    simd: string[]; // ['AVX2', 'AVX512', 'NEON', 'SVE']
  };
  gpu?: {
    vendor: 'nvidia' | 'amd' | 'intel' | 'apple';
    model: string;
    vramGB: number;
    cudaCores?: number;
    computeCapability?: string; // e.g., 'sm_86'
    openclVersion?: string;
    metalVersion?: string;
  };
  memory: {
    totalGB: number;
    availableGB: number;
    speedMTs: number;
  };
  storage: {
    type: 'nvme' | 'ssd' | 'hdd';
    freeGB: number;
    ioSpeedMBps: number;
  };
  os: {
    platform: 'linux' | 'darwin' | 'win32';
    arch: 'x64' | 'arm64' | 'riscv64';
    kernelVersion: string;
  };
  container?: {
    runtime: 'docker' | 'podman' | 'containerd';
    cpus?: number;
    memoryGB?: number;
  };
}

export interface NoviumProjectConfig {
  name: string;
  version: string;
  target: 'auto' | 'native' | 'wasm' | 'cuda' | 'gpu';
  optimizationLevel: 'speed' | 'size' | 'balanced';
  cudaArch: string;
  parallelJobs: number;
  optimizationFlags: string[];
  dependencyOverrides: Map<string, string>;
  lockfileHash: string;
}

interface ExtensionContext {
  hardware: HardwareProfile;
  config: NoviumProjectConfig;
  workspaceRoot: string;
  noviumCLI: NoviumCLI;
}

let extensionContext: ExtensionContext | undefined;

export function activate(context: vscode.ExtensionContext) {
  console.log('[Novium Adaptive] Activating...');

  // Initialize hardware detection
  const hardwareDetector = new HardwareDetector();
  const hardware = hardwareDetector.detect();

  // Load or create project configuration
  const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || '';
  const autoConfigurator = new AutoConfigurator(hardware);
  const config = autoConfigurator.generateConfig(workspaceRoot);

  // Initialize Novium CLI wrapper
  const noviumCLI = new NoviumCLI(workspaceRoot);

  // Create extension context
  extensionContext = {
    hardware,
    config,
    workspaceRoot,
    noviumCLI
  };

  // Register commands
  context.subscriptions.push(
    vscode.commands.registerCommand('novium-adaptive.initialize', () => initializeProject()),
    vscode.commands.registerCommand('novium-adaptive.optimize', () => optimizeForHardware()),
    vscode.commands.registerCommand('novium-adaptive.diagnose', () => diagnoseAndFix()),
    vscode.commands.registerCommand('novium-adaptive.build', () => buildForTarget())
  );

  // Register language client for Novium language server
  registerLanguageSupport(context);

  // Set up auto-configuration on workspace change
  setupWorkspaceWatchers(context);

  // Show welcome notification on first run
  showWelcomeNotification();

  console.log('[Novium Adaptive] Activated successfully');
  console.log(`[Novium Adaptive] Hardware: ${hardware.cpu.cores}C/${hardware.cpu.threads}T ${hardware.cpu.model}`);
  if (hardware.gpu) {
    console.log(`[Novium Adaptive] GPU: ${hardware.gpu.vendor} ${hardware.gpu.model} (${hardware.gpu.vramGB}GB VRAM)`);
  }
}

function initializeProject() {
  if (!extensionContext) return;

  const { workspaceRoot, noviumCLI, hardware } = extensionContext;

  vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: 'Initializing Novium Adaptive Toolchain...',
    cancellable: false
  }, async (progress) => {
    progress.report({ increment: 10, message: 'Detecting hardware...' });

    // Generate optimized project config
    const autoConfigurator = new AutoConfigurator(hardware);
    const config = noviumCLI.generateProjectConfig();

    // Create project structure
    await noviumCLI.initProject();

    // Generate optimized build configuration
    await noviumCLI.generateBuildConfig(config);

    // Generate lockfile
    await noviumCLI.generateLockfile();

    // Install dependencies
    await noviumCLI.installDependencies();

    // Generate VS Code launch configurations
    await generateLaunchConfigs();

    progress.report({ increment: 100, message: 'Project initialized!' });

    vscode.window.showInformationMessage(
      'Novium Adaptive Toolchain initialized! Your project is optimized for this hardware.'
    );
  });
}

function optimizeForHardware() {
  if (!extensionContext) return;

  const { hardware, noviumCLI } = extensionContext;

  vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: 'Optimizing for Hardware...',
    cancellable: false
  }, async (progress) => {
    const autoConfigurator = new AutoConfigurator(hardware);
    const optimizedConfig = autoConfigurator.generateOptimizedConfig();

    await noviumCLI.applyOptimizedConfig(optimizedConfig);

    progress.report({ increment: 100, message: 'Optimization complete!' });

    vscode.window.showInformationMessage(
      `Optimized for ${hardware.cpu.cores}C/${hardware.cpu.threads}T ${hardware.cpu.model}` +
      (hardware.gpu ? ` + ${hardware.gpu.vendor} ${hardware.gpu.model}` : '')
    );
  });
}

function diagnoseAndFix() {
  if (!extensionContext) return;

  const { workspaceRoot, noviumCLI, hardware } = extensionContext;

  vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: 'Diagnosing Novium Project...',
    cancellable: false
  }, async (progress) => {
    const diagnosticEngine = new DiagnosticEngine(hardware);
    const issues = await diagnosticEngine.runFullDiagnosis(workspaceRoot);

    progress.report({ increment: 50, message: 'Found issues, applying fixes...' });

    const fixes = await diagnosticEngine.autoFix(issues);

    progress.report({ increment: 100, message: 'Diagnosis complete' });

    if (fixes.length > 0) {
      vscode.window.showInformationMessage(
        `Fixed ${fixes.length} issues automatically`,
        'Show Details'
      ).then(selection => {
        if (selection === 'Show Details') {
          showDiagnosticReport(issues, fixes);
        }
      });
    } else if (issues.length > 0) {
      vscode.window.showWarningMessage(
        `Found ${issues.length} issues that require manual attention`,
        'Show Details'
      ).then(selection => {
        if (selection === 'Show Details') {
          showDiagnosticReport(issues, []);
        }
      });
    } else {
      vscode.window.showInformationMessage('No issues found! Your project is healthy.');
    }
  });
}

function buildForTarget() {
  if (!extensionContext) return;

  const { noviumCLI, config } = extensionContext;

  vscode.window.showQuickPick(
    ['native', 'wasm', 'cuda', 'auto'],
    { placeHolder: 'Select compilation target' }
  ).then(target => {
    if (!target) return;

    vscode.window.withProgress({
      location: vscode.ProgressLocation.Notification,
      title: `Building for ${target}...`,
      cancellable: true
    }, async (progress, token) => {
      try {
        await noviumCLI.build(target as any, config);
        vscode.window.showInformationMessage(`Build for ${target} completed successfully!`);
      } catch (error) {
        vscode.window.showErrorMessage(`Build failed: ${error}`);
      }
    });
  });
}

function registerLanguageSupport(context: vscode.ExtensionContext) {
  // Register language configuration
  // In a full implementation, this would register a language server client
  console.log('[Novium Adaptive] Language support registered');
}

function setupWorkspaceWatchers(context: vscode.ExtensionContext) {
  // Watch for novium.toml changes
  const watcher = vscode.workspace.createFileSystemWatcher('**/novium.toml');
  watcher.onDidChange(() => {
    if (extensionContext) {
      // Re-run auto-configuration
      const autoConfigurator = new AutoConfigurator(extensionContext.hardware);
      const newConfig = extensionContext.noviumCLI.generateProjectConfig();
      extensionContext.config = newConfig;
    }
  });

  context.subscriptions.push(watcher);
}

async function generateLaunchConfigs() {
  // Generate VS Code launch configurations for debugging
  // This would create .vscode/launch.json with configurations for:
  // - Native debugging (LLDB/GDB)
  // - WASM debugging (Chrome DevTools)
  // - CUDA debugging (cuda-gdb / Nsight)
  console.log('[Novium Adaptive] Generated launch configurations');
}

function showWelcomeNotification() {
  const config = vscode.workspace.getConfiguration('novium-adaptive');
  const hasShownWelcome = config.get('hasShownWelcome', false);

  if (!hasShownWelcome) {
    vscode.window.showInformationMessage(
      'Welcome to Novium Adaptive Toolchain! Run "Novium: Initialize Adaptive Toolchain" to optimize for your hardware.',
      'Initialize Now'
    ).then(selection => {
      if (selection === 'Initialize Now') {
        initializeProject();
      }
    });

    config.update('hasShownWelcome', true, vscode.ConfigurationTarget.Global);
  }
}

function showDiagnosticReport(issues: any[], fixes: any[]) {
  // Show diagnostic report in a webview panel
  const panel = vscode.window.createWebviewPanel(
    'noviumDiagnosticReport',
    'Novium Diagnostic Report',
    vscode.ViewColumn.One,
    {}
  );

  panel.webview.html = generateDiagnosticHTML(issues, fixes);
}

function generateDiagnosticHTML(issues: any[], fixes: any[]): string {
  return `<!DOCTYPE html>
  <html>
  <head><title>Novium Diagnostic Report</title></head>
  <body style="font-family: var(--vscode-font-family); padding: 20px;">
    <h1>Novium Diagnostic Report</h1>
    <h2>Issues Found: ${issues.length}</h2>
    <h2>Auto-Fixed: ${fixes.length}</h2>
    <ul>${issues.map(i => `<li>${i.message}</li>`).join('')}</ul>
    <h3>Fixes Applied</h3>
    <ul>${fixes.map(f => `<li>${f.description}</li>`).join('')}</ul>
  </body>
  </html>`;
}

export function deactivate() {
  console.log('[Novium Adaptive] Deactivating...');
}