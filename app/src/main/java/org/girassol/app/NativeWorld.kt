package org.girassol.app

object NativeWorld {
    init { System.loadLibrary("girassol_jni") }
    external fun nativeSnapshot(): String
    external fun nativeAdvance(hours: Double): String
    external fun nativeDialogue(citizenId: Long): String
}
