package org.girassol.app

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

class WorldDatabase(context: Context) : SQLiteOpenHelper(context, "girassol.db", null, 1) {
    init { setWriteAheadLoggingEnabled(true) }

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL("CREATE TABLE snapshots (id INTEGER PRIMARY KEY AUTOINCREMENT, created_at INTEGER NOT NULL, payload TEXT NOT NULL)")
        db.execSQL("CREATE INDEX snapshots_created_at ON snapshots(created_at)")
    }
    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        // Migrations must be additive and versioned before the schema version is increased.
        if (oldVersion != newVersion) throw IllegalStateException("No migration registered for schema change")
    }
    fun saveSnapshot(payload: String, retain: Int = 48) {
        require(retain > 0)
        writableDatabase.beginTransaction()
        try {
            val values = android.content.ContentValues().apply {
                put("created_at", System.currentTimeMillis())
                put("payload", payload)
            }
            writableDatabase.insertOrThrow("snapshots", null, values)
            writableDatabase.execSQL("DELETE FROM snapshots WHERE id NOT IN (SELECT id FROM snapshots ORDER BY id DESC LIMIT ?)", arrayOf(retain))
            writableDatabase.setTransactionSuccessful()
        } finally { writableDatabase.endTransaction() }
    }
    fun latestSnapshot(): String? = readableDatabase.rawQuery(
        "SELECT payload FROM snapshots ORDER BY id DESC LIMIT 1", null
    ).use { cursor -> if (cursor.moveToFirst()) cursor.getString(0) else null }
}
