/*
 * Copyright 2021 Benjamin Edgington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

/**
 * @file fft_fr.c
 *
 * Discrete fourier transforms over arrays of field elements.
 *
 * Also known as [number theoretic
 * transforms](https://en.wikipedia.org/wiki/Discrete_Fourier_transform_(general)#Number-theoretic_transform).
 *
 * @remark Functions here work only for lengths that are a power of two.
 */

#include "fft_fr.h"
#include "utility.h"

/**
 * Fast Fourier Transform.
 *
 * Recursively divide and conquer.
 *
 * @param[out] out    The results (array of length @p n)
 * @param[in]  in     The input data (array of length @p n * @p stride)
 * @param[in]  stride The input data stride
 * @param[in]  roots  Roots of unity (array of length @p n * @p roots_stride)
 * @param[in]  roots_stride The stride interval among the roots of unity
 * @param[in]  n      Length of the FFT, must be a power of two
 */
static void fft_fr_fast(fr_t *out, const fr_t *in, uint64_t stride, const fr_t *roots, uint64_t roots_stride,
                        uint64_t n) {
    uint64_t half = n / 2;
    if (half > 0) { // Tunable parameter
        fft_fr_fast(out, in, stride * 2, roots, roots_stride * 2, half);
        fft_fr_fast(out + half, in + stride, stride * 2, roots, roots_stride * 2, half);
        for (uint64_t i = 0; i < half; i++) {
            fr_t y_times_root;
            fr_mul(&y_times_root, &out[i + half], &roots[i * roots_stride]);
            fr_sub(&out[i + half], &out[i], &y_times_root);
            fr_add(&out[i], &out[i], &y_times_root);
        }
    } else {
        *out = *in;
    }
}

/**
 * The main entry point for forward and reverse FFTs over the finite field.
 *
 * @param[out] out     The results (array of length @p n)
 * @param[in]  in      The input data (array of length @p n)
 * @param[in]  inverse `false` for forward transform, `true` for inverse transform
 * @param[in]  n       Length of the FFT, must be a power of two
 * @param[in]  fs      Pointer to previously initialised FFTSettings structure with `max_width` at least @p n.
 * @retval C_CZK_OK      All is well
 * @retval C_CZK_BADARGS Invalid parameters were supplied
 */
C_KZG_RET fft_fr(fr_t *out, const fr_t *in, bool inverse, uint64_t n, const FFTSettings *fs) {
    uint64_t stride = fs->max_width / n;
    CHECK(n <= fs->max_width);
    CHECK(is_power_of_two(n));
    if (inverse) {
        fr_t inv_len;
        fr_from_uint64(&inv_len, n);
        fr_inv(&inv_len, &inv_len);
        fft_fr_fast(out, in, 1, fs->reverse_roots_of_unity, stride, n);
        for (uint64_t i = 0; i < n; i++) {
            fr_mul(&out[i], &out[i], &inv_len);
        }
    } else {
        fft_fr_fast(out, in, 1, fs->expanded_roots_of_unity, stride, n);
    }
    return C_KZG_OK;
}

#ifdef KZGTEST

#include "../inc/acutest.h"
#include "test_util.h"

const uint64_t inv_fft_expected[][4] = {
    {0x7fffffff80000008L, 0xa9ded2017fff2dffL, 0x199cec0404d0ec02L, 0x39f6d3a994cebea4L},
    {0xef296e7ffb8ca216L, 0xd5b902cbcef9c1b6L, 0xf06dfe5c7fca260dL, 0x13993b7d05187205L},
    {0xe930fdda2306c7d4L, 0x40e02aff48e2b16bL, 0x83a712d1dd818c8fL, 0x5dbc603bc53c7a3aL},
    {0xf9925986d0d25e90L, 0xcdf85d0a339d7782L, 0xee7a9a5f0410e423L, 0x2e0d216170831056L},
    {0x80007fff80000000L, 0x1fe05202bb00adffL, 0x6045d26b3fd26e6bL, 0x39f6d3a994cebea4L},
    {0x27325dd08ac4cee9L, 0xcbb94f168ddacca9L, 0x6843be68485784b1L, 0x5a6faf9039451673L},
    {0xe92ffdda2306c7d4L, 0x54dd2afcd2dfb16bL, 0xf6554603677e87beL, 0x5dbc603bc53c7a39L},
    {0x1cc772c9b57f126fL, 0xfb73f4d33d3116ddL, 0x4f9388c8d80abcf9L, 0x3ffbc9abcdda7821L},
    {0x7fffffff80000000L, 0xa9ded2017fff2dffL, 0x199cec0404d0ec02L, 0x39f6d3a994cebea4L},
    {0xe3388d354a80ed91L, 0x5849af2fc2cd4521L, 0xe3a64f3f31971b0bL, 0x33f1dda75bc30526L},
    {0x16d00224dcf9382cL, 0xfee079062d1eaa93L, 0x3ce49204a2235046L, 0x163147176461030eL},
    {0xd8cda22e753b3117L, 0x880454ec72238f55L, 0xcaf6199fc14a5353L, 0x197df7c2f05866d4L},
    {0x7fff7fff80000000L, 0x33dd520044fdadffL, 0xd2f4059cc9cf699aL, 0x39f6d3a994cebea3L},
    {0x066da6782f2da170L, 0x85c546f8cc60e47cL, 0x44bf3da90590f3e1L, 0x45e085f1b91a6cf1L},
    {0x16cf0224dcf9382cL, 0x12dd7903b71baa93L, 0xaf92c5362c204b76L, 0x163147176461030dL},
    {0x10d6917f04735deaL, 0x7e04a13731049a48L, 0x42cbd9ab89d7b1f7L, 0x60546bd624850b42L}};

/**
 * Slow Fourier Transform.
 *
 * This is simple, and ok for small sizes. It's mostly useful for testing.
 *
 * @param[out] out    The results (array of length @p n)
 * @param[in]  in     The input data (array of length @p n * @p stride)
 * @param[in]  stride The input data stride
 * @param[in]  roots  Roots of unity (array of length @p n * @p roots_stride)
 * @param[in]  roots_stride The stride interval among the roots of unity
 * @param[in]  n      Length of the FFT, must be a power of two
 */
static void fft_fr_slow(fr_t *out, const fr_t *in, uint64_t stride, const fr_t *roots, uint64_t roots_stride,
                        uint64_t n) {
    fr_t v, last, jv, r;
    for (uint64_t i = 0; i < n; i++) {
        fr_mul(&last, &in[0], &roots[0]);
        for (uint64_t j = 1; j < n; j++) {
            jv = in[j * stride];
            r = roots[((i * j) % n) * roots_stride];
            fr_mul(&v, &jv, &r);
            fr_add(&last, &last, &v);
        }
        out[i] = last;
    }
}

void compare_sft_fft(void) {
    // Initialise: ascending values of i (could be anything), and arbitrary size
    unsigned int size = 12;
    FFTSettings fs;
    TEST_CHECK(new_fft_settings(&fs, size) == C_KZG_OK);
    fr_t data[fs.max_width], out0[fs.max_width], out1[fs.max_width];
    for (int i = 0; i < fs.max_width; i++) {
        fr_from_uint64(data + i, i);
    }

    // Do both fast and slow transforms
    fft_fr_slow(out0, data, 1, fs.expanded_roots_of_unity, 1, fs.max_width);
    fft_fr_fast(out1, data, 1, fs.expanded_roots_of_unity, 1, fs.max_width);

    // Verify the results are identical
    for (int i = 0; i < fs.max_width; i++) {
        TEST_CHECK(fr_equal(out0 + i, out1 + i));
    }

    free_fft_settings(&fs);
}

void roundtrip_fft(void) {
    // Initialise: ascending values of i, and arbitrary size
    unsigned int size = 12;
    FFTSettings fs;
    TEST_CHECK(new_fft_settings(&fs, size) == C_KZG_OK);
    fr_t data[fs.max_width], coeffs[fs.max_width];
    for (int i = 0; i < fs.max_width; i++) {
        fr_from_uint64(data + i, i);
    }

    // Forward and reverse FFT
    TEST_CHECK(fft_fr(coeffs, data, false, fs.max_width, &fs) == C_KZG_OK);
    TEST_CHECK(fft_fr(data, coeffs, true, fs.max_width, &fs) == C_KZG_OK);

    // Verify that the result is still ascending values of i
    for (int i = 0; i < fs.max_width; i++) {
        fr_t tmp;
        fr_from_uint64(&tmp, i);
        TEST_CHECK(fr_equal(&tmp, data + i));
    }

    free_fft_settings(&fs);
}

void inverse_fft(void) {
    // Initialise: ascending values of i
    FFTSettings fs;
    TEST_CHECK(new_fft_settings(&fs, 4) == C_KZG_OK);
    fr_t data[fs.max_width], out[fs.max_width];
    for (int i = 0; i < fs.max_width; i++) {
        fr_from_uint64(&data[i], i);
    }

    // Inverst FFT
    TEST_CHECK(fft_fr(out, data, true, fs.max_width, &fs) == C_KZG_OK);

    // Verify against the known result, `inv_fft_expected`
    int n = sizeof inv_fft_expected / sizeof inv_fft_expected[0];
    TEST_CHECK(n == fs.max_width);
    for (int i = 0; i < n; i++) {
        fr_t expected;
        fr_from_uint64s(&expected, inv_fft_expected[i]);
        TEST_CHECK(fr_equal(&expected, &out[i]));
    }

    free_fft_settings(&fs);
}

void stride_fft(void) {
    unsigned int size1 = 9, size2 = 12;
    uint64_t width = size1 < size2 ? (uint64_t)1 << size1 : (uint64_t)1 << size2;
    FFTSettings fs1, fs2;
    TEST_CHECK(new_fft_settings(&fs1, size1) == C_KZG_OK);
    TEST_CHECK(new_fft_settings(&fs2, size2) == C_KZG_OK);
    fr_t data[width], coeffs1[width], coeffs2[width];
    for (int i = 0; i < width; i++) {
        fr_from_uint64(data + i, i);
    }

    TEST_CHECK(fft_fr(coeffs1, data, false, width, &fs1) == C_KZG_OK);
    TEST_CHECK(fft_fr(coeffs2, data, false, width, &fs2) == C_KZG_OK);

    for (int i = 0; i < width; i++) {
        TEST_CHECK(fr_equal(coeffs1 + i, coeffs2 + i));
    }

    free_fft_settings(&fs1);
    free_fft_settings(&fs2);
}

TEST_LIST = {
    {"FFT_FR_TEST", title},
    {"compare_sft_fft", compare_sft_fft},
    {"roundtrip_fft", roundtrip_fft},
    {"inverse_fft", inverse_fft},
    {"stride_fft", stride_fft},

    {NULL, NULL} /* zero record marks the end of the list */
};

#endif // KZGTEST