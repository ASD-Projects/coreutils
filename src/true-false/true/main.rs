//! ASD CoreUtils true
//! 
//! A fast, flexible and functional replacement for GNU CoreUtils true command.
//! 
//! Copyright (c) 2025 AnmiTaliDev
//! Licensed under the Apache License, Version 2.0

use std::process;

/// Main function that always returns with exit code 0
fn main() {
    // The true command's sole purpose is to exit with success status (0)
    process::exit(0);
}