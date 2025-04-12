// ASD CoreUtils - whoami
// Copyright (c) 2025 AnmiTaliDev
// Licensed under the Apache License, Version 2.0

use std::env;
use std::process::{Command, exit};
use std::time::Instant;
use clap::{Arg, Command as ClapCommand};

const VERSION: &str = "1.0.0";

fn main() {
    let start_time = Instant::now();
    
    let matches = ClapCommand::new("whoami")
        .version(VERSION)
        .about("ASD CoreUtils whoami - Display effective user name")
        .author("AnmiTaliDev")
        .arg(Arg::new("verbose")
            .short('v')
            .long("verbose")
            .help("Display additional information"))
        .arg(Arg::new("time")
            .short('t')
            .long("time")
            .help("Display execution time"))
        .arg(Arg::new("user-only")
            .short('u')
            .long("user-only")
            .help("Display only the username without additional info"))
        .get_matches();

    let verbose = matches.contains_id("verbose");
    let show_time = matches.contains_id("time");
    let user_only = matches.contains_id("user-only");

    // Get username using platform-specific methods
    let username = get_username();
    
    match username {
        Ok(name) => {
            if user_only {
                println!("{}", name);
            } else {
                println!("{}", name);
                
                if verbose {
                    print_verbose_info();
                }
            }
            
            if show_time {
                let elapsed = start_time.elapsed();
                eprintln!("Execution time: {:.6} ms", elapsed.as_secs_f64() * 1000.0);
            }
        },
        Err(err) => {
            eprintln!("Error: {}", err);
            exit(1);
        }
    }
}

#[cfg(unix)]
fn get_username() -> Result<String, String> {
    use std::ffi::CStr;
    use libc::{getuid, getpwuid_r, passwd, geteuid};
    use std::ptr;
    use std::mem;

    unsafe {
        let uid = geteuid();
        let mut pwd: passwd = mem::zeroed();
        let mut result: *mut passwd = ptr::null_mut();
        let mut buffer = vec![0; 16384]; // Buffer for storing pwd data
        
        let ret = getpwuid_r(uid, &mut pwd, buffer.as_mut_ptr(), buffer.len(), &mut result);
        
        if result.is_null() {
            if ret == 0 {
                return Err("User not found".to_string());
            } else {
                return Err(format!("Error retrieving user info, code: {}", ret));
            }
        }
        
        let username = CStr::from_ptr(pwd.pw_name).to_string_lossy().into_owned();
        Ok(username)
    }
}

#[cfg(windows)]
fn get_username() -> Result<String, String> {
    match env::var("USERNAME") {
        Ok(name) => Ok(name),
        Err(_) => {
            // Fallback for Windows if env var is not available
            let output = Command::new("whoami")
                .output()
                .map_err(|e| e.to_string())?;
            
            if output.status.success() {
                let username = String::from_utf8_lossy(&output.stdout)
                    .trim()
                    .to_string();
                Ok(username)
            } else {
                Err("Failed to determine username".to_string())
            }
        }
    }
}

fn print_verbose_info() {
    #[cfg(unix)]
    {
        // Print UID and GID information on Unix-like systems
        unsafe {
            let uid = libc::getuid();
            let euid = libc::geteuid();
            let gid = libc::getgid();
            let egid = libc::getegid();
            
            println!("User ID (UID): {}", uid);
            println!("Effective User ID (EUID): {}", euid);
            println!("Group ID (GID): {}", gid);
            println!("Effective Group ID (EGID): {}", egid);
        }
    }
    
    #[cfg(windows)]
    {
        // Print SID information on Windows
        if let Ok(output) = Command::new("wmic").args(["useraccount", "where", "name=$env:username", "get", "sid"]).output() {
            if output.status.success() {
                let sid = String::from_utf8_lossy(&output.stdout);
                let sid = sid.lines().nth(1).unwrap_or("Unknown").trim();
                println!("Security Identifier (SID): {}", sid);
            }
        }
    }
    
    // Print system information
    println!("Operating System: {}", env::consts::OS);
    println!("Architecture: {}", env::consts::ARCH);
}