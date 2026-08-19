// ============================================================================
// novium_lsp.go — Novium Language Server Protocol Server
// ============================================================================
//
// Implements the Language Server Protocol (LSP) for Novium code.
// Provides syntax highlighting, type diagnostics, auto-completion, and
// other editor features for .nvm, .nvi, .nvw files.
//
// Protocol: https://microsoft.github.io/language-server-protocol/
//
// ============================================================================

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"strings"
)

// ── LSP Constants ──────────────────────────────────────────────────────────

const (
	// LSP Server Info
	LSPName    = "novium"
	LSPVersion = "0.2.0"

	// Capabilities
	CapabilityTextDocumentSync = "textDocumentSync"
	CapabilityHover            = "hover"
	CapabilityCompletion       = "completion"
	CapabilityDefinition       = "definition"
	CapabilityReferences       = "references"
	CapabilityDocumentSymbol   = "documentSymbol"

	// Text document sync kind
	SyncKindNone        = 0
	SyncKindFull        = 1
	SyncKindIncremental = 2

	// Root path placeholder
	RootPathPlaceholder = "$workspaceFolder"
)

// ── LSP Message Types ─────────────────────────────────────────────────────

// Message is an LSP message (notification or request)
type Message struct {
	ID      *integer `json:"id,omitempty"`
	Method  string   `json:"method"`
	Params  json.RawMessage `json:"params,omitempty"`
	Result  json.RawMessage `json:"result,omitempty"`
	Error   *Error `json:"error,omitempty"`
}

// Error is an LSP error
type Error struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

// integer is a helper type for *integer in JSON
type integer int

// Initialize is the initial LSP handshake message
type Initialize struct {
	ProcessID     *integer  `json:"processId,omitempty"`
	RootURI       *string   `json:"rootUri,omitempty"`
	Capabilities  json.RawMessage `json:"capabilities,omitempty"`
	Trace         string `json:"trace,omitempty"`
	WorkspaceFolders *[]string `json:"workspaceFolders,omitempty"`
}

// InitializeResult represents the server capabilities response
type InitializeResult struct {
	Capabilities *ServerCapabilities `json:"capabilities"`
}

// ServerCapabilities defines what the server supports
type ServerCapabilities struct {
	TextDocumentSync *TextDocumentSyncCapability `json:"textDocumentSync,omitempty"`
	Hover            *HoverCapability `json:"hover,omitempty"`
	Completion       *CompletionCapability `json:"completion,omitempty"`
	Definition       *DefinitionCapability `json:"definition,omitempty"`
}

// TextDocumentSyncCapability configures how documents are synced
type TextDocumentSyncCapability struct {
	Kind int `json:"kind,omitempty"` // 1 = full, 2 = incremental
	OpenClose *bool `json:"openClose,omitempty"`
}

// HoverCapability enables hover support
type HoverCapability struct {
	// Whether hover is supported
}

// CompletionCapability enables completion support
type CompletionCapability struct {
	CompletionItem *CompletionItemCapability `json:"completionItem,omitempty"`
}

// CompletionItemCapability configures completion items
type CompletionItemCapability struct {
	// Whether supported
}

// DefinitionCapability enables definition support
type DefinitionCapability struct {
	// Whether supported
}

// CompletionItem represents a completion item
type CompletionItem struct {
	Label      string   `json:"label"`
	Kind       *integer `json:"kind,omitempty"`
	Detail     string   `json:"detail,omitempty"`
	Documentation string `json:"documentation,omitempty"`
	// Additional text edit operations
}

// HoverResult represents hover information
type HoverResult struct {
	Contents json.RawMessage `json:"contents"`
}

// DidOpenTextDocument is sent when a text document opens
type DidOpenTextDocument struct {
	TextDocument DidOpenTextDocumentParams `json:"textDocument"`
}

// DidOpenTextDocumentParams describes a text document that opened
type DidOpenTextDocumentParams struct {
	TextDocument DidOpenTextDocumentIdentifier `json:"textDocument"`
	Text         string `json:"text"`
	Annotation  string `json:"annotation,omitempty"`
}

// DidOpenTextDocumentIdentifier identifies a text document
type DidOpenTextDocumentIdentifier struct {
	URI string `json:"uri"`
}

// Completed is sent when the client finishes launching
type Completed struct{}

// InitializeRequest is the initial client request
type InitializeRequest struct {
	ID      *integer `json:"id"`
	Method  string   `json:"method"`
	Params  Initialize `json:"params"`
}

// HoverRequest represents a hover request
type HoverRequest struct {
	ID      *integer `json:"id"`
	Method  string   `json:"method"`
	Params  HoverParams `json:"params"`
}

// HoverParams describes the parameters for a hover request
type HoverParams struct {
	TextDocument Position `json:"textDocument"`
	Position   Position `json:"position"`
}

// Position represents a position in a text document
type Position struct {
	Line      integer `json:"line"`
	Character integer `json:"character"`
}

// CompletionRequest represents a completion request
type CompletionRequest struct {
	ID       *integer `json:"id"`
	Method   string   `json:"method"`
	Params   CompletionParams `json:"params"`
	Response bool     `json:"-"`
}

// CompletionParams describes the parameters for a completion request
type CompletionParams struct {
	TextDocument Position `json:"textDocument"`
	Position   Position `json:"position"`
	Context      CompletionContext `json:"context"`
}

// CompletionContext describes the completion context
type CompletionContext struct {
	TriggerKind integer `json:"triggerKind"`
	// Additional trigger info
}

// CompletionItemKind represents the kind of completion item
type CompletionItemKind integer

const (
	CompletionItemKindFile       CompletionItemKind = 1
	CompletionItemKindModule     CompletionItemKind = 2
	CompletionItemKindFunction   CompletionItemKind = 3
	CompletionItemKindMethod     CompletionItemKind = 4
	CompletionItemKindProperty   CompletionItemKind = 5
	CompletionItemKindField      CompletionItemKind = 6
	CompletionItemKindConstructor CompletionItemKind = 7
	CompletionItemKindVariable   CompletionItemKind = 8
	CompletionItemKindClass      CompletionItemKind = 9
	CompletionItemKindInterface  CompletionItemKind = 10
	CompletionItemKindConstant   CompletionItemKind = 11
	CompletionItemKindEnum       CompletionItemKind = 12
)

// String returns the human-readable kind
func (k CompletionItemKind) String() string {
	switch k {
	case CompletionItemKindFile:
		return "file"
	case CompletionItemKindModule:
		return "module"
	case CompletionItemKindFunction:
		return "function"
	case CompletionItemKindMethod:
		return "method"
	case CompletionItemKindProperty:
		return "property"
	case CompletionItemKindField:
		return "field"
	case CompletionItemKindConstructor:
		return "constructor"
	case CompletionItemKindVariable:
		return "variable"
	case CompletionItemKindClass:
		return "class"
	case CompletionItemKindInterface:
		return "interface"
	case CompletionItemKindConstant:
		return "constant"
	case CompletionItemKindEnum:
		return "enum"
	default:
		return "default"
	}
}

// CompletionList represents a list of completion items
type CompletionList struct {
	IsIncomplete bool            `json:"isIncomplete,omitempty"`
	Items        []CompletionItem `json:"items"`
}

// InitializeResponse is the response to an initialize request
type InitializeResponse struct {
	ID      *integer          `json:"id"`
	Result   InitializeResult `json:"result"`
	Error    *Error           `json:"error,omitempty"`
}

// HoverResponse is the response to a hover request
type HoverResponse struct {
	ID      *integer       `json:"id"`
	Result  *HoverResult   `json:"result,omitempty"`
	Error   *Error         `json:"error,omitempty"`
}

// CompletionResponse is the response to a completion request
type CompletionResponse struct {
	ID      *integer         `json:"id"`
	Result  *CompletionList `json:"result,omitempty"`
	Error   *Error          `json:"error,omitempty"`
}

// Notification is an LSP notification (no response expected)
type Notification struct {
	Method  string          `json:"method"`
	Params  json.RawMessage `json:"params"`
}

// Reader reads LSP messages from a reader
type Reader struct {
	reader *bufio.Reader
}

// Writer writes LSP messages to a writer
type Writer struct {
	writer *bufio.Writer
}

// NewReader creates a new LSP message reader
func NewReader(r *bufio.Reader) *Reader {
	return &Reader{reader: r}
}

// NewWriter creates a new LSP message writer
func NewWriter(w *bufio.Writer) *Writer {
	return &Writer{writer: w}
}

// ReadMessage reads the next LSP message
func (r *Reader) ReadMessage() (*Message, error) {
	// Read header line
	header, err := r.reader.ReadString('\n')
	if err != nil {
		return nil, err
	}
	header = strings.TrimSpace(header)

	// Parse Content-Length
	var contentLength int
	fmt.Sscanf(header, "Content-Length: %d", &contentLength)

	// Read blank line
	_, err = r.reader.ReadString('\n')
	if err != nil {
		return nil, err
	}

	// Read content
	content := make([]byte, contentLength)
	_, err = r.reader.Read(content)
	if err != nil {
		return nil, err
	}

	// Parse JSON
	var msg Message
	err = json.Unmarshal(content, &msg)
	if err != nil {
		return nil, err
	}

	return &msg, nil
}

// WriteMessage writes an LSP message
func (w *Writer) WriteMessage(msg *Message) error {
	// Marshal to JSON
	content, err := json.Marshal(msg)
	if err != nil {
		return err
	}

	// Write Content-Length header
	header := fmt.Sprintf("Content-Length: %d\r\n\r\n", len(content))

	// Write to writer
	_, err = w.writer.WriteString(header + string(content))
	if err != nil {
		return err
	}

	return w.writer.Flush()
}

// HandleInitialize handles the initialize request
func HandleInitialize(msg *Message, writer *Writer) {
	// Build capabilities
	caps := ServerCapabilities{
		TextDocumentSync: &TextDocumentSyncCapability{
			Kind: SyncKindFull,
		},
		Hover:      &HoverCapability{},
		Completion: &CompletionCapability{CompletionItem: &CompletionItemCapability{}},
		Definition: &DefinitionCapability{},
	}

	initResult := InitializeResponse{
		ID:     msg.ID,
		Result: InitializeResult{Capabilities: &caps},
		Error:  nil,
	}

	content, err := json.Marshal(initResult)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error marshaling init response: %v\n", err)
		return
	}

	resp := Message{
		ID:     msg.ID,
		Result: json.RawMessage(content),
		Error:  nil,
	}

	if err := writer.WriteMessage(&resp); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing init response: %v\n", err)
	}
}

// HandleTextDocumentPublishDiagnostics handles publishing diagnostics
func HandleTextDocumentPublishDiagnostics(_ *Message, _ *Writer) {
	// Placeholder - diagnostics sent from client to server
}

// HandleDidOpenTextDocument handles text document did open
func HandleDidOpenTextDocument(msg *Message, writer *Writer) {
	params := DidOpenTextDocumentParams{}
	json.Unmarshal(msg.Params, &params)

	// In a full implementation, we would parse the Novium source
	// and compute diagnostics immediately
	// For now, just acknowledge

	// Send empty initialize to confirm
	resp := Message{
		Method: "window/logMessage",
		Params: json.RawMessage(`{"type":"note","message":"novium LSP: document opened"}`),
	}
	if err := writer.WriteMessage(&resp); err != nil {
		fmt.Fprintf(os.Stderr, "Error sending log message: %v\n", err)
	}
}

// HandleTextDocumentCompletion handles completion requests
func HandleTextDocumentCompletion(msg *Message, writer *Writer) {
	params := CompletionParams{}
	json.Unmarshal(msg.Params, &params)

	// Generate completion items based on Novium keywords and standard library
	items := []CompletionItem{}
	kind := integer(CompletionItemKindFunction)

	// Add Novium keywords
	keywords := []string{"fn", "let", "var", "if", "elif", "else", "while", "match", "try", "catch", "return", "go", "async", "await"}
	for _, kw := range keywords {
		items = append(items, CompletionItem{
			Label:         kw,
			Kind:          &kind,
			Detail:        "keyword",
			Documentation: "Novium language keyword",
		})
	}

	// Add standard library functions (from math.nvm and string.nvm)
	mathFuncs := []string{"abs", "signum", "clamp", "lerp", "sqrt", "pow", "cbrt",
		"deg_to_rad", "rad_to_deg", "sin", "cos", "tan", "sinh", "cosh", "tanh"}
	for _, f := range mathFuncs {
		items = append(items, CompletionItem{
			Label:         f,
			Kind:          &kind,
			Detail:        "standard library",
			Documentation: "Standard library function from math.nvm",
		})
	}

	stringFuncs := []string{"str_len", "str_concat", "str_contains", "to_upper", "to_lower",
		"str_split", "str_starts_with", "str_ends_with", "html_escape", "html_unescape"}
	for _, f := range stringFuncs {
		items = append(items, CompletionItem{
			Label:         f,
			Kind:          &kind,
			Detail:        "standard library",
			Documentation: "Standard library function from string.nvm",
		})
	}

	result := CompletionList{
		IsIncomplete: false,
		Items:        items,
	}

	response := CompletionResponse{
		ID:     msg.ID,
		Result: &result,
		Error:  nil,
	}

	content, err := json.Marshal(response)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error marshaling completion response: %v\n", err)
		return
	}
	msg.Result = json.RawMessage(content)
	if err := writer.WriteMessage(msg); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing completion response: %v\n", err)
	}
}

// HandleTextDocumentHover handles hover requests
func HandleTextDocumentHover(msg *Message, writer *Writer) {
	params := HoverParams{}
	json.Unmarshal(msg.Params, &params)

	// In a full implementation, we would:
	// 1. Parse the Novium source at the given position
	// 2. Determine the expression/type at that position
	// 3. Return type information or documentation

	// For now, return a basic hover with the keyword under cursor

	// Create a simple hover with the word at position
	word := "novium function" // Placeholder
	hoverDoc := fmt.Sprintf("## %s\n\nPlaceholder hover documentation for Novium at line %d, character %d.",
		word, params.Position.Line+1, params.Position.Character+1)

	result := HoverResult{
		Contents: json.RawMessage(`"` + hoverDoc + `"`),
	}

	response := HoverResponse{
		ID:     msg.ID,
		Result: &result,
		Error:  nil,
	}

	content, err := json.Marshal(response)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error marshaling hover response: %v\n", err)
		return
	}
	msg.Result = json.RawMessage(content)
	if err := writer.WriteMessage(msg); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing hover response: %v\n", err)
	}
}

// Main function: run LSP server over stdin/stdout
func main() {
	reader := NewReader(bufio.NewReader(os.Stdin))
	writer := NewWriter(bufio.NewWriter(os.Stdout))

	// Send initialize response
	initMsg := Message{
		Method: "initialize",
	}
	if err := writer.WriteMessage(&initMsg); err != nil {
		fmt.Fprintf(os.Stderr, "Error sending initialize: %v\n", err)
		return
	}

	// Send initialized notification
	initialized := Message{
		Method: "initialized",
		Params: json.RawMessage(`{}`),
	}
	if err := writer.WriteMessage(&initialized); err != nil {
		fmt.Fprintf(os.Stderr, "Error sending initialized: %v\n", err)
		return
	}

	// Main message loop
	for {
		msg, err := reader.ReadMessage()
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading message: %v\n", err)
			break
		}

		switch msg.Method {
		case "initialized":
			// No action needed
		case "textDocument/didOpen":
			HandleDidOpenTextDocument(msg, writer)
		case "textDocument/publishDiagnostics":
			HandleTextDocumentPublishDiagnostics(msg, writer)
		case "textDocument/completion":
			HandleTextDocumentCompletion(msg, writer)
		case "textDocument/hover":
			HandleTextDocumentHover(msg, writer)
		case "textDocument/definition":
			// Placeholder - return default position
			defResp := Message{
				Method: "textDocument/definition",
				Result: json.RawMessage(`{"uri":"","range":{"start":{"line":0,"character":0},"end":{"line":0,"character":0}}}`),
			}
			if err := writer.WriteMessage(&defResp); err != nil {
				fmt.Fprintf(os.Stderr, "Error writing definition: %v\n", err)
			}
		case "textDocument/references":
			// Placeholder
		default:
			fmt.Fprintf(os.Stderr, "Unknown method: %s\n", msg.Method)
		}
	}
}