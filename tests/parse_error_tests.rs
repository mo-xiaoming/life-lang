use pretty_assertions::assert_eq;
use std::fs;
use std::process::Command;

const TEST_DATA_DIR: &str = "tests/file_test/testdata";

fn check_error_msg(source_path: &std::path::Path, expected: &str) {
    let output = Command::new("cargo")
        .args([
            "run",
            "-q",
            "--bin",
            "llangc",
            "--",
            source_path.to_str().unwrap(),
        ])
        .output()
        .expect("Failed to execute command");
    let got = String::from_utf8(output.stderr).unwrap_or_default();
    assert_eq!(
        got,
        expected,
        "Failed to compare `{}`",
        source_path.display()
    );
}

#[test]
fn parse_error_tests() {
    let mut entries = fs::read_dir(TEST_DATA_DIR)
        .unwrap_or_else(|e| panic!("Failed to read directory `{}`: {}", TEST_DATA_DIR, e))
        .collect::<Result<Vec<_>, _>>()
        .unwrap_or_else(|e| panic!("Failed to read directory `{}`: {}", TEST_DATA_DIR, e));
    entries.sort_by_key(|e| e.path());

    for entry in entries {
        let source_path = entry.path();
        if source_path.is_file() && source_path.extension().unwrap_or_default() == "llang" {
            let expected_path = source_path.with_extension("expected");
            check_error_msg(
                &source_path,
                &fs::read_to_string(expected_path).unwrap_or_default(),
            );
        }
    }
}
