// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_JNI_INT_WRAPPER_H_
#define BASE_ANDROID_JNI_INT_WRAPPER_H_

// Wrapper used to receive int when calling Java from native.
// The wrapper disallows automatic conversion of long to int.
// This is to avoid a common anti-pattern where a Java int is used
// to receive a native pointer. Please use a Java long to receive
// native pointers, so that the code works on both 32-bit and 64-bit
// platforms. Note the wrapper allows other lossy conversions into
// jint that could be consider anti-patterns, such as from size_t.

// Checking is only done in debugging builds.

#ifdef NDEBUG

typedef jint JniIntWrapper;

// This inline is sufficiently trivial that it does not change the
// final code generated by g++.
inline jint as_jint(JniIntWrapper wrapper) {
  return wrapper;
}

#else

class JniIntWrapper {
 public:
  JniIntWrapper() : i_(0) {}
  JniIntWrapper(int i) : i_(i) {}
  JniIntWrapper(const JniIntWrapper& ji) : i_(ji.i_) {}
  template <class T> JniIntWrapper(const T& t) : i_(t) {}
  jint as_jint() const { return i_; }
 private:
  // If you get an "is private" error at the line below it is because you used
  // an implicit conversion to convert a long to an int when calling Java.
  // We disallow this, as a common anti-pattern allows converting a native
  // pointer (intptr_t) to a Java int. Please use a Java long to represent
  // a native pointer. If you want a lossy conversion, please use an
  // explicit conversion in your C++ code. Note an error is only seen when
  // compiling on a 64-bit platform, as intptr_t is indistinguishable from
  // int on 32-bit platforms.
  JniIntWrapper(long);
  jint i_;
};

inline jint as_jint(const JniIntWrapper& wrapper) {
  return wrapper.as_jint();
}

#endif  // NDEBUG

#endif  // BASE_ANDROID_JNI_INT_WRAPPER_H_