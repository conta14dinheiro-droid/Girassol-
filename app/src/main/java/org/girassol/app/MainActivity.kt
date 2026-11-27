package org.girassol.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.ui.graphics.Color

class MainActivity : ComponentActivity() {
    private lateinit var database: WorldDatabase
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        database = WorldDatabase(this)
        val first = database.latestSnapshot() ?: NativeWorld.nativeSnapshot().also(database::saveSnapshot)
        setContent {
            MaterialTheme(colorScheme = darkColorScheme(
                background = Color.Black, surface = Color(0xFF09090B), primary = Color(0xFFFFC857)
            )) {
                WorldScreen(initialSnapshot = first, onPersist = database::saveSnapshot)
            }
        }
    }
}
