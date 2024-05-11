package com.android.example.cameraxapp

import android.content.Intent
import android.os.Bundle
import android.widget.ArrayAdapter
import android.widget.ListView
import androidx.appcompat.app.AppCompatActivity

class ActivitySelector : AppCompatActivity() {

    private val activities = arrayOf(
        "Resampling",
        "Filters",
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_selector)

        val adapter = ArrayAdapter(this, R.layout.activity_selector_list_item, activities)
        findViewById<ListView>(R.id.activity_list)
            .also {
                it.setAdapter(adapter)
                it.setOnItemClickListener { parent, view, position, id ->
                    val targetActivity = when (position) {
                        0 -> ResamplingActivity::class.java
                        1 -> FiltersActivity::class.java
                        else -> null
                    }

                    if (targetActivity != null) {
                        val intent = Intent(this, targetActivity)
                        startActivity(intent)
                    }
                }
            }
    }
}