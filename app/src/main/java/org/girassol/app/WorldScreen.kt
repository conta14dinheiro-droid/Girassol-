package org.girassol.app

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import org.json.JSONObject

@Composable
fun WorldScreen(initialSnapshot: String, onPersist: (String) -> Unit) {
    var snapshot by remember { mutableStateOf(initialSnapshot) }
    var dialogue by remember { mutableStateOf("A mata respira ao redor do mundo.") }
    val state = remember(snapshot) { JSONObject(snapshot) }
    Scaffold(containerColor = MaterialTheme.colorScheme.background, bottomBar = {
        NavigationBar(containerColor = MaterialTheme.colorScheme.surface, modifier = Modifier.padding(horizontal = 24.dp, vertical = 8.dp)) {
            NavigationBarItem(selected = true, onClick = {}, icon = { Text("◎") }, label = { Text("Mundo") })
            NavigationBarItem(selected = false, onClick = {}, icon = { Text("◌") }, label = { Text("Vida") })
            NavigationBarItem(selected = false, onClick = {}, icon = { Text("◇") }, label = { Text("Diário") })
        }
    }) { padding ->
        Column(Modifier.fillMaxSize().padding(padding).padding(20.dp), verticalArrangement = Arrangement.spacedBy(14.dp)) {
            Text("GIRASSOL · MUNDO", style = MaterialTheme.typography.titleLarge)
            Text("Dia ${state.optLong("day")} · ${"%.1f".format(state.optDouble("hour"))} h")
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                MetricCard("Temperatura", "${"%.1f".format(state.optDouble("temperatureC"))} °C", Modifier.weight(1f))
                MetricCard("Umidade", "${"%.0f".format(state.optDouble("humidity") * 100)}%", Modifier.weight(1f))
            }
            MetricCard("Água superficial", "${"%.0f".format(state.optDouble("surfaceWater") * 100)}%", Modifier.fillMaxWidth())
            MetricCard("Mata nativa", "${"%.0f".format(state.optDouble("canopy") * 100)}%", Modifier.fillMaxWidth())
            Card(shape = RoundedCornerShape(24.dp), colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
                Column(Modifier.padding(18.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("Diálogo local · acessível")
                    Text(dialogue)
                    Text("A fala é apresentada em texto. Cidadãos mudos usam texto/gestos; cidadãos surdos não dependem de áudio.", style = MaterialTheme.typography.bodySmall)
                }
            }
            Button(onClick = {
                snapshot = NativeWorld.nativeAdvance(1.0)
                onPersist(snapshot)
                dialogue = "O ciclo avançou uma hora. ${if (state.optDouble("surfaceWater") > 0.35) "Há água no território." else "A água está escassa."}"
            }) { Text("Avançar 1 hora") }
        }
    }
}

@Composable
private fun MetricCard(title: String, value: String, modifier: Modifier = Modifier) {
    Card(modifier = modifier, shape = RoundedCornerShape(24.dp), colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
        Column(Modifier.padding(18.dp)) {
            Text(title, style = MaterialTheme.typography.labelMedium)
            Text(value, style = MaterialTheme.typography.headlineSmall)
        }
    }
}
