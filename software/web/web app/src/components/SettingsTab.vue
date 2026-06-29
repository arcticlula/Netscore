<script setup>
import { defineProps, defineEmits, ref, watch, computed } from 'vue'
import { NSwitch, NRadio, NRadioGroup, NSpace, NButton } from 'naive-ui'

const props = defineProps({
  isBigBoard: Boolean,
  enableBuzzer: Boolean,
  displayMode: Number,
  slots: Number,
  swapTeams: Boolean,
  bleName: String,
  battery: Object,
  serveBypass: Number
})

const emit = defineEmits(['update:settings', 'update:bleName', 'update:serveBypass'])

const localIsBigBoard = ref(props.isBigBoard)
const localEnableBuzzer = ref(props.enableBuzzer)
const localDisplayMode = ref(props.displayMode)
const localSwapTeams = ref(props.swapTeams)
const localBleName = ref(props.bleName)

watch(() => props.isBigBoard, (val) => localIsBigBoard.value = val)
watch(() => props.enableBuzzer, (val) => localEnableBuzzer.value = val)
watch(() => props.displayMode, (val) => localDisplayMode.value = val)
watch(() => props.swapTeams, (val) => localSwapTeams.value = val)
watch(() => props.bleName, (val) => localBleName.value = val)

const serveBypassEnabled = ref(props.serveBypass !== -1)
const localServeBypass = ref(props.serveBypass !== -1 ? props.serveBypass : 2)

watch(() => props.serveBypass, (val) => {
  serveBypassEnabled.value = (val !== -1)
  if (val !== -1) {
    localServeBypass.value = val
  }
})

const onBypassToggle = (enabled) => {
  if (enabled) {
    emit('update:serveBypass', localServeBypass.value)
  } else {
    emit('update:serveBypass', -1)
  }
}

const triggerBypassUpdate = (val) => {
  localServeBypass.value = val
  emit('update:serveBypass', val)
}

const triggerUpdate = () => {
  emit('update:settings', {
    board_type: localIsBigBoard.value ? 1 : 0,
    buzzer: localEnableBuzzer.value,
    display_mode: localDisplayMode.value,
    swap_teams: localSwapTeams.value
  })
}

const slotStatusText = computed(() => {
  switch (props.slots) {
    case 0: return 'Side A'
    case 1: return 'Side B'
    case 2: return 'Both (A & B)'
    case 3: return 'None'
    default: return 'Unknown'
  }
})

const isDisplayModeDisabled = computed(() => {
  return props.slots === 0 || props.slots === 1 || props.slots === 3
})

const triggerBleNameUpdate = () => {
  emit('update:bleName', localBleName.value)
}

</script>

<template>
  <div class="settings-tab">
    <div class="setting-item">
      <div style="display: flex; flex-direction: column; gap: 8px;">
        <label>Bluetooth Name</label>
        <span style="font-size: 0.85em; opacity: 0.7;">This will disconnect the app and require you to pair again.</span>
      </div>
      <div style="display: flex; gap: 8px;">
        <input type="text" v-model="localBleName" class="text-input" maxlength="28" />
        <n-button type="primary" @click="triggerBleNameUpdate">Apply</n-button>
      </div>
    </div>

    <div class="setting-item" style="flex-direction: column; align-items: flex-start; gap: 12px;">
      <label>Device Batteries</label>
      <div style="display: flex; width: 100%; justify-content: space-between; gap: 12px;">
        <div class="battery-card">
          <span class="bat-label">Main Board</span>
          <span class="bat-val">{{ battery?.main || 0 }}%</span>
        </div>
        <div class="battery-card">
          <span class="bat-label">Controller 1</span>
          <span class="bat-val">{{ battery?.device_1 ? battery.device_1 + '%' : 'N/A' }}</span>
        </div>
        <div class="battery-card">
          <span class="bat-label">Controller 2</span>
          <span class="bat-val">{{ battery?.device_2 ? battery.device_2 + '%' : 'N/A' }}</span>
        </div>
      </div>
      <div v-if="battery?.min !== undefined && battery?.min !== null && battery?.max !== undefined && battery?.max !== null" style="display: flex; width: 100%; justify-content: space-between; font-size: 0.85em; opacity: 0.7; padding-top: 4px; border-top: 1px solid rgba(255, 255, 255, 0.05);">
        <span>NVS Min Limit: <strong>{{ battery.min }} mV</strong></span>
        <span>NVS Max Limit: <strong>{{ battery.max }} mV</strong></span>
      </div>
    </div>

    <div class="setting-item">
      <label>Connected Slots</label>
      <span class="status-badge">{{ slotStatusText }}</span>
    </div>
    
    <div class="setting-item">
      <label>Display Mode</label>
      <select v-model="localDisplayMode" @change="triggerUpdate" :disabled="isDisplayModeDisabled">
        <option :value="0">Both Sides</option>
        <option :value="1">Side A Only</option>
        <option :value="2">Side B Only</option>
      </select>
    </div>

    <div class="setting-item" style="flex-direction: column; align-items: flex-start; gap: 12px;">
      <div style="display: flex; justify-content: space-between; width: 100%; align-items: center;">
        <label>Skip Serve Selection</label>
        <n-switch v-model:value="serveBypassEnabled" @update:value="onBypassToggle">
          <template #checked>On</template>
          <template #unchecked>Off</template>
        </n-switch>
      </div>
      <div v-if="serveBypassEnabled" style="width: 100%; padding-top: 4px;">
        <n-radio-group v-model:value="localServeBypass" name="serveBypassGroup" @update:value="triggerBypassUpdate">
          <n-space>
            <n-radio :value="0">Home Serves</n-radio>
            <n-radio :value="1">Away Serves</n-radio>
            <n-radio :value="2">Random</n-radio>
          </n-space>
        </n-radio-group>
      </div>
    </div>


    <div class="setting-item">
      <label>Swap Teams (Home on Right)</label>
      <n-switch v-model:value="localSwapTeams" @update:value="triggerUpdate">
        <template #checked>On</template>
        <template #unchecked>Off</template>
      </n-switch>
    </div>

    <div class="setting-item">
      <label>Board Type</label>
      <select v-model="localIsBigBoard" @change="triggerUpdate">
        <option :value="false">Small Board</option>
        <option :value="true">Big Board</option>
      </select>
    </div>

    <div class="setting-item">
      <label>Buzzer</label>
      <n-switch v-model:value="localEnableBuzzer" @update:value="triggerUpdate">
        <template #checked>On</template>
        <template #unchecked>Off</template>
      </n-switch>
    </div>
  </div>
</template>

<style scoped>
.settings-tab {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
  padding: 1rem;
}

.setting-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1rem;
  background-color: var(--surface-color);
  border-radius: 8px;
}

.setting-item label {
  font-weight: 500;
  color: var(--text-color);
}

select {
  padding: 0.5rem 1rem;
  border-radius: 6px;
  background-color: var(--bg-color);
  color: var(--text-color);
  border: 1px solid var(--border-color);
  outline: none;
}

select:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.status-badge {
  background-color: var(--primary-color, #18a058);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 0.9em;
  font-weight: 500;
}

.text-input {
  padding: 0.5rem 1rem;
  border-radius: 6px;
  background-color: var(--bg-color);
  color: var(--text-color);
  border: 1px solid var(--border-color);
  outline: none;
  width: 150px;
}

.battery-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: rgba(255, 255, 255, 0.05);
  padding: 12px 8px;
  border-radius: 8px;
}

.battery-card .bat-label {
  font-size: 0.8em;
  opacity: 0.8;
  margin-bottom: 4px;
}

.battery-card .bat-val {
  font-size: 1.2em;
  font-weight: bold;
  color: #63e2b7;
}

</style>
