#! /bin/bash

script_dir="$(dirname "$0")"

echo "BUILDING TESTS..."
(cd $script_dir && make clean)
(cd $script_dir && make unit_tests)
echo "IN-MEMORY TESTS BUILT!!"
echo ""

tests_bin="$script_dir/bin"

run_test() {
    local test_path="$1"

    if [ -x "$test_path" ]; then
        echo ""
        echo "@@@@@@@@@@@@@@@ $(basename "$test_path") @@@@@@@@@@@@@@@"
        $test_path
        status=$?
        echo "--------------------------------------------------------"
        echo ""
        if [ $status -ne 0 ]; then
            echo "!!!!! Test: $(basename "$test_path") failed !!!!!"
            exit 0
        fi
    else
        echo "$test_path is not executable."
    fi
}

if [ $# -eq 0 ]; then
    echo "RUNNING ALL TESTS!!"
else
    echo "RUNNING TESTS: $@"
fi

for file in "$tests_bin"/*; do
    if [ -f "$file" ]; then
        if [ $# -eq 0 ]; then
            run_test $file
        else
            for arg in "$@"; do
                if [ "$tests_bin/$arg" = "$file" ]; then
                    run_test $file
                fi
            done
        fi
    fi
done
