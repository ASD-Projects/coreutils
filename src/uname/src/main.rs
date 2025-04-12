// ASD CoreUtils - uname utility
// Copyright (c) 2025 AnmiTaliDev
// Licensed under the Apache License, Version 2.0

use clap::{Arg, ArgAction, Command};
use std::env;
use std::process;

#[cfg(target_os = "linux")]
extern crate libc;

fn main() {
    let matches = Command::new("uname")
        .version("1.0.0")
        .author("AnmiTaliDev")
        .about("ASD CoreUtils uname - display system information")
        .arg(
            Arg::new("all")
                .short('a')
                .long("all")
                .help("Print all information")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("kernel-name")
                .short('s')
                .long("kernel-name")
                .help("Print the kernel name")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("nodename")
                .short('n')
                .long("nodename")
                .help("Print the network node hostname")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("kernel-release")
                .short('r')
                .long("kernel-release")
                .help("Print the kernel release")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("kernel-version")
                .short('v')
                .long("kernel-version")
                .help("Print the kernel version")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("machine")
                .short('m')
                .long("machine")
                .help("Print the machine hardware name")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("processor")
                .short('p')
                .long("processor")
                .help("Print the processor type")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("hardware-platform")
                .short('i')
                .long("hardware-platform")
                .help("Print the hardware platform")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("operating-system")
                .short('o')
                .long("operating-system")
                .help("Print the operating system")
                .action(ArgAction::SetTrue),
        )
        .get_matches();

    let sys_info = get_system_info();
    
    // If no arguments provided or --all specified, show kernel name (system) by default
    let no_args = !matches.get_flag("all") && 
                  !matches.get_flag("kernel-name") &&
                  !matches.get_flag("nodename") &&
                  !matches.get_flag("kernel-release") &&
                  !matches.get_flag("kernel-version") &&
                  !matches.get_flag("machine") &&
                  !matches.get_flag("processor") &&
                  !matches.get_flag("hardware-platform") &&
                  !matches.get_flag("operating-system");

    if no_args {
        println!("{}", sys_info.kernel_name);
        return;
    }

    let mut output = Vec::new();

    if matches.get_flag("all") || matches.get_flag("kernel-name") {
        output.push(sys_info.kernel_name);
    }

    if matches.get_flag("all") || matches.get_flag("nodename") {
        output.push(sys_info.nodename);
    }

    if matches.get_flag("all") || matches.get_flag("kernel-release") {
        output.push(sys_info.kernel_release);
    }

    if matches.get_flag("all") || matches.get_flag("kernel-version") {
        output.push(sys_info.kernel_version);
    }

    if matches.get_flag("all") || matches.get_flag("machine") {
        output.push(sys_info.machine);
    }

    if matches.get_flag("all") || matches.get_flag("processor") {
        output.push(sys_info.processor);
    }

    if matches.get_flag("all") || matches.get_flag("hardware-platform") {
        output.push(sys_info.hardware_platform);
    }

    if matches.get_flag("all") || matches.get_flag("operating-system") {
        output.push(sys_info.operating_system);
    }

    println!("{}", output.join(" "));
}

struct SystemInfo {
    kernel_name: String,
    nodename: String,
    kernel_release: String,
    kernel_version: String,
    machine: String,
    processor: String,
    hardware_platform: String,
    operating_system: String,
}

#[cfg(target_os = "linux")]
fn get_system_info() -> SystemInfo {
    use std::ffi::CStr;
    
    unsafe {
        let mut utsname: libc::utsname = std::mem::zeroed();
        if libc::uname(&mut utsname) != 0 {
            eprintln!("Failed to get system information");
            process::exit(1);
        }

        let kernel_name = CStr::from_ptr(utsname.sysname.as_ptr()).to_string_lossy().into_owned();
        let nodename = CStr::from_ptr(utsname.nodename.as_ptr()).to_string_lossy().into_owned();
        let kernel_release = CStr::from_ptr(utsname.release.as_ptr()).to_string_lossy().into_owned();
        let kernel_version = CStr::from_ptr(utsname.version.as_ptr()).to_string_lossy().into_owned();
        let machine = CStr::from_ptr(utsname.machine.as_ptr()).to_string_lossy().into_owned();
        
        // Get processor info from /proc/cpuinfo
        let processor = match std::fs::read_to_string("/proc/cpuinfo") {
            Ok(contents) => {
                contents
                    .lines()
                    .find(|line| line.starts_with("model name"))
                    .and_then(|line| line.split(':').nth(1))
                    .map(|s| s.trim().to_string())
                    .unwrap_or_else(|| "unknown".to_string())
            },
            Err(_) => "unknown".to_string(),
        };

        // Hardware platform - can be same as machine in some cases
        let hardware_platform = machine.clone();
        
        // Operating system detection
        let operating_system = if std::path::Path::new("/etc/os-release").exists() {
            match std::fs::read_to_string("/etc/os-release") {
                Ok(contents) => {
                    contents
                        .lines()
                        .find(|line| line.starts_with("PRETTY_NAME="))
                        .and_then(|line| {
                            let parts: Vec<&str> = line.splitn(2, '=').collect();
                            if parts.len() == 2 {
                                Some(parts[1].trim_matches('"').to_string())
                            } else {
                                None
                            }
                        })
                        .unwrap_or_else(|| "Linux".to_string())
                },
                Err(_) => "Linux".to_string(),
            }
        } else {
            "Linux".to_string()
        };

        SystemInfo {
            kernel_name,
            nodename,
            kernel_release,
            kernel_version,
            machine,
            processor,
            hardware_platform,
            operating_system,
        }
    }
}

#[cfg(not(target_os = "linux"))]
fn get_system_info() -> SystemInfo {
    eprintln!("This version of uname only supports Linux systems");
    process::exit(1);
}