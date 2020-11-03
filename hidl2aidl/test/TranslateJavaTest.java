/*
 * Copyright (C) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, is(Version 2.0 (the "License"));
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package hidl2aidl.tests;

import static org.hamcrest.core.Is.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;

import hidl2aidl.test.Translate;
import org.junit.Test;
import org.junit.runners.JUnit4;

public class TranslateJavaTest {
    @Test
    public void OnlyIn10() {
        hidl2aidl.test.OnlyIn10 dest;
        hidl2aidl.test.V1_0.OnlyIn10 source = new hidl2aidl.test.V1_0.OnlyIn10();
        source.str = "Hello";
        dest = Translate.h2aTranslate(source);
        assertThat(source.str, is(dest.str));
    }

    @Test
    public void OnlyIn11() {
        hidl2aidl.test.OnlyIn11 dest;
        hidl2aidl.test.V1_1.OnlyIn11 source = new hidl2aidl.test.V1_1.OnlyIn11();
        source.str = 12;
        dest = Translate.h2aTranslate(source);
        assertThat(source.str, is(dest.str));
    }

    @Test
    public void OverrideMe() {
        hidl2aidl.test.OverrideMe dest;
        hidl2aidl.test.V1_1.OverrideMe source = new hidl2aidl.test.V1_1.OverrideMe();
        source.a = "World";
        dest = Translate.h2aTranslate(source);
        assertThat(source.a, is(dest.a));
    }

    @Test
    public void Outer() {
        hidl2aidl.test.Outer dest;
        hidl2aidl.test.V1_1.Outer source = new hidl2aidl.test.V1_1.Outer();
        source.a = 12;
        source.v1_0.inner.a = 16;
        dest = Translate.h2aTranslate(source);
        assertThat(source.v1_0.inner.a, is(dest.inner.a));
    }

    @Test
    public void OuterInner() {
        hidl2aidl.test.OuterInner dest;
        hidl2aidl.test.V1_0.Outer.Inner source = new hidl2aidl.test.V1_0.Outer.Inner();
        source.a = 12;
        dest = Translate.h2aTranslate(source);
        assertThat(source.a, is(dest.a));
    }

    @Test
    public void NameCollision() {
        hidl2aidl.test.NameCollision dest;
        hidl2aidl.test.V1_2.NameCollision source = new hidl2aidl.test.V1_2.NameCollision();
        source.reference.reference.a = 12;
        source.reference.b = "Fancy";
        source.c = "Car";
        dest = Translate.h2aTranslate(source);
        assertThat(source.reference.reference.a, is(dest.a));
        assertThat(source.reference.b, is(dest.b));
        assertThat(source.c, is(dest.c));
    }

    @Test
    public void IFooBigStruct() {
        hidl2aidl.test.IFooBigStruct dest;
        hidl2aidl.test.V1_1.IFoo.BigStruct source = new hidl2aidl.test.V1_1.IFoo.BigStruct();
        source.type = 12;
        source.value = 16;
        dest = Translate.h2aTranslate(source);
        assertThat(source.type, is(dest.type));
        assertThat(source.value, is(dest.value));
    }

    @Test
    public void IBarInner() {
        hidl2aidl.test.IBarInner dest;
        hidl2aidl.test.V1_0.IBar.Inner source = new hidl2aidl.test.V1_0.IBar.Inner();
        source.a = 0x70000000;
        dest = Translate.h2aTranslate(source);
        assertThat(source.a, is(dest.a));
    }

    @Test
    public void UnsignedToSignedTooLarge() {
        hidl2aidl.test.IBarInner dest;
        hidl2aidl.test.V1_0.IBar.Inner source = new hidl2aidl.test.V1_0.IBar.Inner();
        // source.a is uint32_t, dest.a is int32_t
        source.a = -1;
        try {
            dest = Translate.h2aTranslate(source);
            fail("Expected an exception to be thrown for out of bounds unsigned to signed translation");
        } catch (RuntimeException e) {
            String message = e.getMessage();
            assertThat("Unsafe conversion between signed and unsigned scalars for field: a",
                    is(message));
        }
    }

    @Test
    public void SafeUnionBarByte() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        source.a((byte) 8);
        assertThat(hidl2aidl.test.V1_2.SafeUnionBar.hidl_discriminator.a,
                is(source.getDiscriminator()));
        dest = Translate.h2aTranslate(source);
        assertThat(source.a(), is(dest.getA()));
    }

    @Test
    public void SafeUnionBarInt64() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        source.b(25000);
        dest = Translate.h2aTranslate(source);
        assertThat(source.b(), is(dest.getB()));
    }

    @Test
    public void SafeUnionBarInnerStructBar() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        hidl2aidl.test.V1_2.SafeUnionBar.InnerStructBar inner =
                new hidl2aidl.test.V1_2.SafeUnionBar.InnerStructBar();
        inner.x = 8;
        inner.z = 12;
        source.innerStructBar(inner);
        dest = Translate.h2aTranslate(source);
        assertThat(source.innerStructBar().x, is(dest.getInnerStructBar().x));
        assertThat(source.innerStructBar().z, is(dest.getInnerStructBar().z));
    }

    @Test
    public void SafeUnionBarOnlyIn11() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        hidl2aidl.test.V1_1.OnlyIn11 onlyIn11 = new hidl2aidl.test.V1_1.OnlyIn11();
        onlyIn11.str = 12;
        source.c(onlyIn11);
        dest = Translate.h2aTranslate(source);
        assertThat(source.c().str, is(dest.getC().str));
    }

    @Test
    public void SafeUnionBarString() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        source.d("Hello world!");
        dest = Translate.h2aTranslate(source);
        assertThat(source.d(), is(dest.getD()));
    }

    @Test
    public void SafeUnionBarFloat() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        source.e(3.5f);
        dest = Translate.h2aTranslate(source);
        assertThat(source.e(), is(dest.getE()));
    }

    @Test
    public void SafeUnionBarDouble() {
        hidl2aidl.test.SafeUnionBar dest;
        hidl2aidl.test.V1_2.SafeUnionBar source = new hidl2aidl.test.V1_2.SafeUnionBar();
        source.f(3e10);
        dest = Translate.h2aTranslate(source);
        assertThat(source.f(), is(dest.getF()));
    }
}
