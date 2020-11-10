///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package hidl2aidl.test;
@VintfStability
interface IFoo {
  void multipleInputs(in String in1, in String in2);
  void multipleInputsAndOutputs(in String in1, in String in2, out hidl2aidl.test.IFooBigStruct out1, out hidl2aidl.test.IFooBigStruct out2);
  void multipleOutputs(out hidl2aidl.test.IFooBigStruct out1, out hidl2aidl.test.IFooBigStruct out2);
  String oneOutput();
  String removedOutput();
  void someBar(in String a, in String b);
  oneway void someFoo(in byte a);
  void useImportedStruct(in hidl2aidl.test.Outer outer);
  hidl2aidl.test.IFooBigStruct useStruct();
  void versionTest_(in String a);
  boolean versionTest_two_(in String a);
}
