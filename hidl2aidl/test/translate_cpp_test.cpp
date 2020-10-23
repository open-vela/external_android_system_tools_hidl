/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hidl2aidl/test/translate-cpp.h>

#include <gtest/gtest.h>

class Hidl2aidlTranslateTest : public ::testing::Test {};

namespace android {

TEST_F(Hidl2aidlTranslateTest, OnlyIn10) {
    bool ret;
    hidl2aidl::test::OnlyIn10 dest;
    hidl2aidl::test::V1_0::OnlyIn10 source;
    source.str = "Hello";
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.str, String8(dest.str).c_str());
}

TEST_F(Hidl2aidlTranslateTest, OnlyIn11) {
    bool ret;
    hidl2aidl::test::OnlyIn11 dest;
    hidl2aidl::test::V1_1::OnlyIn11 source;
    source.str = 12;
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.str, dest.str);
}

TEST_F(Hidl2aidlTranslateTest, OverrideMe) {
    bool ret;
    hidl2aidl::test::OverrideMe dest;
    hidl2aidl::test::V1_1::OverrideMe source;
    source.a = "World";
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.a, String8(dest.a).c_str());
}

TEST_F(Hidl2aidlTranslateTest, Outer) {
    bool ret;
    hidl2aidl::test::Outer dest;
    hidl2aidl::test::V1_1::Outer source;
    source.a = 12;
    source.v1_0.inner.a = 16;
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.a, dest.a);
    EXPECT_EQ(source.v1_0.inner.a, dest.inner.a);
}

TEST_F(Hidl2aidlTranslateTest, OuterInner) {
    bool ret;
    hidl2aidl::test::OuterInner dest;
    hidl2aidl::test::V1_0::Outer::Inner source;
    source.a = 12;
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.a, dest.a);
}

TEST_F(Hidl2aidlTranslateTest, NameCollision) {
    bool ret;
    hidl2aidl::test::NameCollision dest;
    hidl2aidl::test::V1_2::NameCollision source;
    source.reference.reference.a = 12;
    source.reference.b = "Fancy";
    source.c = "Car";
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.reference.reference.a, dest.a);
    EXPECT_EQ(source.reference.b, String8(dest.b).c_str());
    EXPECT_EQ(source.c, String8(dest.c).c_str());
}

TEST_F(Hidl2aidlTranslateTest, IFooBigStruct) {
    bool ret;
    hidl2aidl::test::IFooBigStruct dest;
    hidl2aidl::test::V1_1::IFoo::BigStruct source;
    source.type = 12;
    source.value = 16;
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.type, dest.type);
    EXPECT_EQ(source.value, dest.value);
}

TEST_F(Hidl2aidlTranslateTest, IBarInner) {
    bool ret;
    hidl2aidl::test::IBarInner dest;
    hidl2aidl::test::V1_0::IBar::Inner source;
    source.a = 0x70000000;
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ((int32_t)source.a, dest.a);
}

TEST_F(Hidl2aidlTranslateTest, UnsignedToSignedTooLarge) {
    bool ret;
    hidl2aidl::test::IBarInner dest;
    hidl2aidl::test::V1_0::IBar::Inner source;
    // source.a is uint32_t, dest.a is int32_t
    source.a = 0xf0000000;
    ret = h2a::translate(source, &dest);
    EXPECT_FALSE(ret);
    EXPECT_NE((int32_t)source.a, dest.a);
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarByte) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    source.a(8);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.a(), dest.get<hidl2aidl::test::SafeUnionBar::a>());
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarInt64) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    source.b(25000);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.b(), (uint64_t)dest.get<hidl2aidl::test::SafeUnionBar::b>());
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarInnerStructBar) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    hidl2aidl::test::V1_2::SafeUnionBar::InnerStructBar inner;
    inner.x = 8;
    inner.z = 12;
    source.innerStructBar(inner);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.innerStructBar().x,
              dest.get<hidl2aidl::test::SafeUnionBar::innerStructBar>().x);
    EXPECT_EQ(source.innerStructBar().z,
              dest.get<hidl2aidl::test::SafeUnionBar::innerStructBar>().z);
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarOnlyIn11) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    hidl2aidl::test::V1_1::OnlyIn11 onlyIn11;
    onlyIn11.str = 12;
    source.c(onlyIn11);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.c().str, dest.get<hidl2aidl::test::SafeUnionBar::c>().str);
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarString) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    source.d("Hello world!");
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.d(), String8(dest.get<hidl2aidl::test::SafeUnionBar::d>()).c_str());
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarFloat) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    source.e(3.5f);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.e(), dest.get<hidl2aidl::test::SafeUnionBar::e>());
}

TEST_F(Hidl2aidlTranslateTest, SafeUnionBarDouble) {
    bool ret;
    hidl2aidl::test::SafeUnionBar dest;
    hidl2aidl::test::V1_2::SafeUnionBar source;
    source.f(3e10);
    ret = h2a::translate(source, &dest);
    EXPECT_TRUE(ret);
    EXPECT_EQ(source.f(), dest.get<hidl2aidl::test::SafeUnionBar::f>());
}

}  // namespace android
