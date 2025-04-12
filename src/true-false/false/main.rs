//! ASD CoreUtils - false
//! 
//! A modern, fast, and functional implementation of the 'false' utility
//! that always returns a non-zero exit code.
//!
//! Author: AnmiTaliDev
//! License: Apache License 2.0

fn main() {
    // Always exit with status code 1 (failure)
    std::process::exit(1);
}