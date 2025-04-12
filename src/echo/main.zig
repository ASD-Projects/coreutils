const std = @import("std");

/// ASD CoreUtils Echo - A faster, more functional alternative to GNU CoreUtils echo
/// Author: AnmiTaliDev
/// License: Apache 2.0
///
/// Features:
/// - Supports standard echo functionality (printing arguments with spaces)
/// - -n flag to omit trailing newline
/// - -e flag to enable interpretation of backslash escapes
/// - -E flag to disable interpretation of backslash escapes (default)
/// - Optimized for performance with minimal allocations

const Options = struct {
    interpret_escapes: bool = false,
    newline: bool = true,
    help: bool = false,
    version: bool = false,
};

fn printVersion() void {
    std.debug.print("ASD CoreUtils echo 1.0.0\n", .{});
    std.debug.print("Copyright (C) 2025 AnmiTaliDev\n", .{});
    std.debug.print("License: Apache 2.0\n", .{});
}

fn printHelp() void {
    std.debug.print("Usage: echo [OPTION]... [STRING]...\n", .{});
    std.debug.print("Echo the STRING(s) to standard output.\n\n", .{});
    std.debug.print("  -n             do not output the trailing newline\n", .{});
    std.debug.print("  -e             enable interpretation of backslash escapes\n", .{});
    std.debug.print("  -E             disable interpretation of backslash escapes (default)\n", .{});
    std.debug.print("  --help         display this help and exit\n", .{});
    std.debug.print("  --version      output version information and exit\n\n", .{});
    std.debug.print("If -e is in effect, the following sequences are recognized:\n", .{});
    std.debug.print("  \\\\      backslash\n", .{});
    std.debug.print("  \\a      alert (BEL)\n", .{});
    std.debug.print("  \\b      backspace\n", .{});
    std.debug.print("  \\c      produce no further output\n", .{});
    std.debug.print("  \\e      escape\n", .{});
    std.debug.print("  \\f      form feed\n", .{});
    std.debug.print("  \\n      new line\n", .{});
    std.debug.print("  \\r      carriage return\n", .{});
    std.debug.print("  \\t      horizontal tab\n", .{});
    std.debug.print("  \\v      vertical tab\n", .{});
}

fn getEscapeSequence(escape_char: u8) ?[]const u8 {
    return switch (escape_char) {
        '\\' => "\\",
        'a' => "\x07", // Alert (BEL)
        'b' => "\x08", // Backspace
        'c' => "",     // Produce no further output
        'e' => "\x1B", // Escape
        'f' => "\x0C", // Form feed
        'n' => "\n",   // Newline
        'r' => "\r",   // Carriage return
        't' => "\t",   // Horizontal tab
        'v' => "\x0B", // Vertical tab
        else => null,
    };
}

fn processEscapes(allocator: std.mem.Allocator, input: []const u8) ![]u8 {
    var result = std.ArrayList(u8).init(allocator);
    errdefer result.deinit();
    
    var i: usize = 0;
    while (i < input.len) {
        if (input[i] == '\\' and i + 1 < input.len) {
            const escape_char = input[i + 1];
            if (getEscapeSequence(escape_char)) |replacement| {
                try result.appendSlice(replacement);
                // Special case for \c which stops all output
                if (escape_char == 'c') {
                    break;
                }
            } else {
                // Unknown escape sequence, keep as is
                try result.append('\\');
                try result.append(escape_char);
            }
            i += 2;
        } else {
            try result.append(input[i]);
            i += 1;
        }
    }
    
    return result.toOwnedSlice();
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    const stdout = std.io.getStdOut().writer();
    
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    
    var options = Options{};
    var arg_index: usize = 1;
    
    // Parse options
    while (arg_index < args.len) : (arg_index += 1) {
        const arg = args[arg_index];
        if (!std.mem.startsWith(u8, arg, "-")) {
            break;
        } else if (std.mem.eql(u8, arg, "-n")) {
            options.newline = false;
        } else if (std.mem.eql(u8, arg, "-e")) {
            options.interpret_escapes = true;
        } else if (std.mem.eql(u8, arg, "-E")) {
            options.interpret_escapes = false;
        } else if (std.mem.eql(u8, arg, "--help")) {
            options.help = true;
        } else if (std.mem.eql(u8, arg, "--version")) {
            options.version = true;
        } else {
            break;
        }
    }
    
    if (options.help) {
        printHelp();
        return;
    }
    
    if (options.version) {
        printVersion();
        return;
    }
    
    // Print arguments
    var first = true;
    while (arg_index < args.len) : (arg_index += 1) {
        if (!first) {
            try stdout.writeByte(' ');
        }
        first = false;
        
        if (options.interpret_escapes) {
            const processed = try processEscapes(allocator, args[arg_index]);
            defer allocator.free(processed);
            try stdout.writeAll(processed);
        } else {
            try stdout.writeAll(args[arg_index]);
        }
    }
    
    if (options.newline) {
        try stdout.writeByte('\n');
    }
}