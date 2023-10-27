#! /bin/bash

script_dir="$(dirname "$0")"

echo "BUILDING TESTS..."
(cd $script_dir && make clean_tests)
(cd $script_dir && make tests)
echo "TESTS BUILT!!"
echo ""

test_path="$script_dir/test/bin"

mount_point=$1
echo "ALTFS MOUNTED AT: $mount_point"
shift

run_test() {
    local test_path="$1"

    if [ -x "$test_path" ]; then
        echo ""
        echo "===== $(basename "$test_path") ====="
        $test_path $mount_point
        echo "-----------------------"
        echo ""
    else
        echo "$test_path is not executable."
    fi
}

if [ $# -eq 0 ]; then
    echo "RUNNING ALL TESTS!!"
else
    echo "RUNNING TESTS: $@"
fi

for file in "$test_path"/*; do
    if [ -f "$file" ]; then
        if [ $# -eq 0 ]; then
            run_test $file
        else
            for arg in "$@"; do
                if [ "$test_path/$arg" = "$file" ]; then
                    run_test $file
                fi
            done
        fi
    fi
done