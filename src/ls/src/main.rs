use chrono::{DateTime, Local};
use clap::{App, Arg};
use colored::Colorize;
use std::fs::{self, DirEntry};
use std::io;
use std::os::unix::fs::PermissionsExt;
use std::path::Path;
use std::process;

struct FileInfo {
    name: String,
    size: u64,
    permissions: u32,
    modified: DateTime<Local>,
    is_dir: bool,
    is_symlink: bool,
}

fn main() -> io::Result<()> {
    let matches = App::new("ASD CoreUtils ls")
        .version("1.0.0")
        .author("AnmiTaliDev")
        .about("Fast and flexible ls")
        .arg(
            Arg::with_name("all")
                .short("a")
                .long("all")
                .help("Show hidden files"),
        )
        .arg(
            Arg::with_name("long")
                .short("l")
                .long("long")
                .help("Use long listing format"),
        )
        .arg(
            Arg::with_name("human-readable")
                .short("h")
                .long("human-readable")
                .help("Human readable file sizes"),
        )
        .arg(
            Arg::with_name("sort")
                .short("s")
                .long("sort")
                .takes_value(true)
                .possible_values(&["name", "time", "size"])
                .default_value("name")
                .help("Sort by name, modification time, or size"),
        )
        .arg(
            Arg::with_name("reverse")
                .short("r")
                .long("reverse")
                .help("Reverse sort order"),
        )
        .arg(
            Arg::with_name("recursive")
                .short("R")
                .long("recursive")
                .help("List subdirectories recursively"),
        )
        .arg(
            Arg::with_name("color")
                .long("color")
                .possible_values(&["never", "auto", "always"])
                .default_value("auto")
                .help("When to use color"),
        )
        .arg(
            Arg::with_name("PATH")
                .help("Directory to list")
                .default_value(".")
                .multiple(true),
        )
        .get_matches();

    let show_hidden = matches.is_present("all");
    let long_format = matches.is_present("long");
    let human_readable = matches.is_present("human-readable");
    let sort_by = matches.value_of("sort").unwrap_or("name");
    let reverse = matches.is_present("reverse");
    let recursive = matches.is_present("recursive");
    let use_color = matches.value_of("color").unwrap_or("auto") != "never";
    
    let paths: Vec<&str> = matches.values_of("PATH").unwrap_or_default().collect();
    
    // Use current directory if no paths provided
    let paths = if paths.is_empty() {
        vec!["."]
    } else {
        paths
    };

    let multi_path = paths.len() > 1;
    
    for path in &paths {
        if multi_path {
            println!("\n{}:", path);
        }
        
        match list_directory(
            path,
            show_hidden,
            long_format,
            human_readable,
            sort_by,
            reverse,
            recursive,
            use_color,
            0,
        ) {
            Ok(_) => (),
            Err(e) => {
                eprintln!("Error listing '{}': {}", path, e);
                process::exit(1);
            }
        }
    }

    Ok(())
}

fn list_directory(
    dir_path: &str,
    show_hidden: bool,
    long_format: bool,
    human_readable: bool,
    sort_by: &str,
    reverse: bool,
    recursive: bool,
    use_color: bool,
    depth: usize,
) -> io::Result<()> {
    let path = Path::new(dir_path);
    if !path.is_dir() {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            format!("'{}' is not a directory", dir_path),
        ));
    }

    let indent = if depth > 0 {
        "  ".repeat(depth)
    } else {
        String::new()
    };

    // Get all entries in the directory
    let mut entries: Vec<DirEntry> = fs::read_dir(path)?
        .filter_map(|entry| entry.ok())
        .filter(|entry| {
            show_hidden || !entry
                .file_name()
                .to_string_lossy()
                .starts_with('.')
        })
        .collect();

    // Sort entries
    match sort_by {
        "name" => {
            entries.sort_by(|a, b| {
                let a_filename = a.file_name();
                let b_filename = b.file_name();
                let a_name = a_filename.to_string_lossy();
                let b_name = b_filename.to_string_lossy();
                if reverse {
                    b_name.cmp(&a_name)
                } else {
                    a_name.cmp(&b_name)
                }
            });
        }
        "time" => {
            entries.sort_by(|a, b| {
                let a_time = a.metadata().unwrap().modified().unwrap();
                let b_time = b.metadata().unwrap().modified().unwrap();
                if reverse {
                    b_time.cmp(&a_time)
                } else {
                    a_time.cmp(&b_time)
                }
            });
        }
        "size" => {
            entries.sort_by(|a, b| {
                let a_size = a.metadata().unwrap().len();
                let b_size = b.metadata().unwrap().len();
                if reverse {
                    b_size.cmp(&a_size)
                } else {
                    a_size.cmp(&b_size)
                }
            });
        }
        _ => {}
    }

    let mut files = Vec::new();
    
    for entry in entries {
        let path = entry.path();
        let metadata = entry.metadata()?;
        let name = entry.file_name().to_string_lossy().to_string();
        
        let is_symlink = path.is_symlink();
        let is_dir = path.is_dir();
        
        let modified = DateTime::from(metadata.modified()?);
        
        files.push(FileInfo {
            name,
            size: metadata.len(),
            permissions: metadata.permissions().mode(),
            modified,
            is_dir,
            is_symlink,
        });
    }

    if long_format {
        for file in &files {
            let permissions = format_permissions(file.permissions);
            let modified_time = file.modified.format("%b %d %H:%M").to_string();
            let size = if human_readable {
                format_size(file.size)
            } else {
                file.size.to_string()
            };
            
            let file_name = format_name(&file.name, file.is_dir, file.is_symlink, use_color);
            
            println!(
                "{}{} {:>8} {} {}",
                indent, permissions, size, modified_time, file_name
            );
        }
    } else {
        for file in &files {
            let file_name = format_name(&file.name, file.is_dir, file.is_symlink, use_color);
            println!("{}{}", indent, file_name);
        }
    }

    // Handle recursive listing
    if recursive {
        for file in &files {
            if file.is_dir {
                let new_path = format!("{}/{}", dir_path, file.name);
                println!("\n{}{}:", indent, new_path);
                let _ = list_directory(
                    &new_path,
                    show_hidden,
                    long_format,
                    human_readable,
                    sort_by,
                    reverse,
                    recursive,
                    use_color,
                    depth + 1,
                );
            }
        }
    }

    Ok(())
}

fn format_permissions(mode: u32) -> String {
    let file_type = match mode & 0o170000 {
        0o040000 => 'd', // directory
        0o120000 => 'l', // symbolic link
        _ => '-',        // regular file
    };

    let user_r = if mode & 0o400 != 0 { 'r' } else { '-' };
    let user_w = if mode & 0o200 != 0 { 'w' } else { '-' };
    let user_x = if mode & 0o100 != 0 { 'x' } else { '-' };

    let group_r = if mode & 0o040 != 0 { 'r' } else { '-' };
    let group_w = if mode & 0o020 != 0 { 'w' } else { '-' };
    let group_x = if mode & 0o010 != 0 { 'x' } else { '-' };

    let other_r = if mode & 0o004 != 0 { 'r' } else { '-' };
    let other_w = if mode & 0o002 != 0 { 'w' } else { '-' };
    let other_x = if mode & 0o001 != 0 { 'x' } else { '-' };

    format!(
        "{}{}{}{}{}{}{}{}{}{}",
        file_type,
        user_r, user_w, user_x,
        group_r, group_w, group_x,
        other_r, other_w, other_x
    )
}

fn format_size(size: u64) -> String {
    const KB: u64 = 1024;
    const MB: u64 = KB * 1024;
    const GB: u64 = MB * 1024;
    const TB: u64 = GB * 1024;

    if size < KB {
        format!("{}B", size)
    } else if size < MB {
        format!("{:.1}K", size as f64 / KB as f64)
    } else if size < GB {
        format!("{:.1}M", size as f64 / MB as f64)
    } else if size < TB {
        format!("{:.1}G", size as f64 / GB as f64)
    } else {
        format!("{:.1}T", size as f64 / TB as f64)
    }
}

fn format_name(name: &str, is_dir: bool, is_symlink: bool, use_color: bool) -> String {
    if !use_color {
        if is_dir {
            format!("{}/", name)
        } else if is_symlink {
            format!("{}@", name)
        } else {
            name.to_string()
        }
    } else {
        if is_dir {
            format!("{}/", name.blue().bold())
        } else if is_symlink {
            format!("{}@", name.cyan())
        } else {
            name.to_string()
        }
    }
}