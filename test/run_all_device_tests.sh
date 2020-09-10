#!/bin/bash

function run() {
    local FAILED_TESTS=()
    local SKIPPED_TESTS=()

    # Tests directly relevant to HIDL infrustructure but aren't
    # located in system/tools/hidl
    local RELATED_RUNTIME_TESTS=(\
        libhidl_test \
    )

    local RUN_TIME_TESTS=(\
        libhidl-gen-utils_test \
    )
    RUN_TIME_TESTS+=(${RELATED_RUNTIME_TESTS[@]})

    local SCRIPT_TESTS=(\
        hidl_test\
        hidl_test_java\
        fmq_test\
    )

    $ANDROID_BUILD_TOP/build/soong/soong_ui.bash --make-mode -j \
        ${RUN_TIME_TESTS[*]} ${SCRIPT_TESTS[*]} || exit 1

    # TODO(b/129507417): build with supported configurations
    mkdir -p $ANDROID_PRODUCT_OUT/data/framework/hidl_test_java.jar || exit 1
    cp $ANDROID_PRODUCT_OUT/testcases/hidl_test_java_java/*/hidl_test_java_java.jar \
       $ANDROID_PRODUCT_OUT/data/framework/hidl_test_java_java.jar || exit 1

    adb sync data || exit 1

    local BITNESS=("nativetest" "nativetest64")

    for test in ${RUN_TIME_TESTS[@]}; do
        local ran_test=0
        for bits in ${BITNESS[@]}; do
            local test_binary="/data/$bits/$test/$test"

            # only run test if binary is installed
            if adb shell sh -c '"[ -f '$test_binary' ]"'; then
                ran_test=1
            else
                SKIPPED_TESTS+=("$bits:$test")
                break
            fi

            echo $test_binary
            adb shell $test_binary || FAILED_TESTS+=("$bits:$test")
        done
        if [ $ran_test -ne 1 ]; then
            FAILED_TESTS+=("$test (not installed)")
        fi
    done

    for test in ${SCRIPT_TESTS[@]}; do
        echo $test
        adb shell /data/nativetest64/$test ||
            FAILED_TESTS+=("$test")
    done

    echo
    echo ===== ALL DEVICE TESTS SUMMARY =====
    echo
    if [ ${#SKIPPED_TESTS[@]} -gt 0 ]; then
        for skipped in ${SKIPPED_TEST[@]}; do
            echo "SKIPPED TEST: $skipped"
        done
    fi
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        for failed in ${FAILED_TESTS[@]}; do
            echo "FAILED TEST: $failed"
        done
        exit 1
    else
        echo "SUCCESS"
    fi
}

run
