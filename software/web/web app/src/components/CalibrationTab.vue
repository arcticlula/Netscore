<script setup>
import { ref, watch, computed } from 'vue'
import { NSpace, NText, NSlider, NDivider, NGrid, NGridItem, NCard, NTabs, NTabPane, NButton, NSwitch } from 'naive-ui'

const props = defineProps({
  groupCal: Object, // { r, g, b }
  segmentA: Array,
  segmentB: Array,
  miscA: Array,
  miscB: Array,
  isBigBoard: Boolean
})

// Display Definitions
const LED_HOME_1 = 10
const LED_HOME_2 = 11
const LED_HOME_3 = 15
const LED_MID = 16
const LED_AWAY_1 = 14
const LED_AWAY_2 = 13
const LED_AWAY_3 = 12
const TIME_COLON_TOP = 20
const TIME_COLON_BOTTOM = 21
const BAR_LED_1 = 22
const BAR_LED_2 = 24
const BAR_LED_3 = 25
const BAR_LED_4 = 23

const emit = defineEmits(['update:group', 'update:digit', 'update:misc', 'import:calibration'])

const isAdvancedMode = ref(false)

const localGroupCal = ref({ ...props.groupCal })
const localSegmentA = ref([...props.segmentA])
const localSegmentB = ref([...props.segmentB])
const localMiscA = ref([...props.miscA])
const localMiscB = ref([...props.miscB])

watch(() => props.groupCal, (newVal) => { localGroupCal.value = { ...newVal } }, { deep: true })
watch(() => props.segmentA, (newVal) => { localSegmentA.value = [...newVal] }, { deep: true })
watch(() => props.segmentB, (newVal) => { localSegmentB.value = [...newVal] }, { deep: true })
watch(() => props.miscA, (newVal) => { localMiscA.value = [...newVal] }, { deep: true })
watch(() => props.miscB, (newVal) => { localMiscB.value = [...newVal] }, { deep: true })

watch(() => props.isBigBoard, (newVal) => {
  if (!newVal) {
    // Zero out extra bar LEDs
    [BAR_LED_2, BAR_LED_3, BAR_LED_4].forEach(idx => {
      if (localMiscA.value[idx] !== 0) {
        localMiscA.value[idx] = 0;
        emit('update:misc', { side: 'A', index: idx, value: 0 });
      }
      if (localMiscB.value[idx] !== 0) {
        localMiscB.value[idx] = 0;
        emit('update:misc', { side: 'B', index: idx, value: 0 });
      }
    });
  }
}, { immediate: true })

const updateGroup = (color, value) => {
  localGroupCal.value[color] = value
  emit('update:group', { color, value })
}

const updateDigit = (side, index, value) => {
  if (side === 'A') localSegmentA.value[index] = value
  else localSegmentB.value[index] = value
  emit('update:digit', { side, index, value })
}

const updateMisc = (side, miscId, value) => {
  if (!props.isBigBoard && [BAR_LED_2, BAR_LED_3, BAR_LED_4].includes(miscId)) {
    value = 0;
  }
  
  if (miscId === 'cluster_home') {
    [LED_HOME_1, LED_HOME_2, LED_HOME_3].forEach(idx => {
      if (side === 'A') localMiscA.value[idx] = value
      else localMiscB.value[idx] = value
      emit('update:misc', { side, index: idx, value })
    })
  } else if (miscId === 'cluster_away') {
    [LED_AWAY_1, LED_AWAY_2, LED_AWAY_3].forEach(idx => {
      if (side === 'A') localMiscA.value[idx] = value
      else localMiscB.value[idx] = value
      emit('update:misc', { side, index: idx, value })
    })
  } else {
    if (side === 'A') localMiscA.value[miscId] = value
    else localMiscB.value[miscId] = value
    emit('update:misc', { side, index: miscId, value })
  }
}

const getLabel = (index) => {
  if (index <= 1) return `Home Points ${index + 1}`
  if (index <= 3) return `Away Points ${index - 1}`
  if (index <= 7) return `Time Digit ${index - 3}`
  if (index == 8) return `Home Sets`
  if (index == 9) return `Away Sets`
  return `Digit ${index}`
}

const miscSliders = [
  { id: 'cluster_home', label: 'Home Clusters' },
  { id: 'cluster_away', label: 'Away Clusters' },
  { id: LED_MID, label: 'Middle Indicator' },
  { id: TIME_COLON_TOP, label: 'Time Colon Top' },
  { id: TIME_COLON_BOTTOM, label: 'Time Colon Bottom' },
  { id: BAR_LED_1, label: 'Bar LED 1' },
  { id: BAR_LED_2, label: 'Bar LED 2' },
  { id: BAR_LED_3, label: 'Bar LED 3' },
  { id: BAR_LED_4, label: 'Bar LED 4' },
]

const getMiscValue = (side, miscId) => {
  if (miscId === 'cluster_home') return side === 'A' ? localMiscA.value[LED_HOME_1] : localMiscB.value[LED_HOME_1]
  if (miscId === 'cluster_away') return side === 'A' ? localMiscA.value[LED_AWAY_1] : localMiscB.value[LED_AWAY_1]
  return side === 'A' ? localMiscA.value[miscId] : localMiscB.value[miscId]
}

const isSliderDisabled = (miscId) => {
  return !props.isBigBoard && [BAR_LED_2, BAR_LED_3, BAR_LED_4].includes(miscId)
}

const simpleDigitSliders = [
  { id: 'home_points', label: 'Home Points (Digits 1 & 2)', indices: [0, 1] },
  { id: 'away_points', label: 'Away Points (Digits 3 & 4)', indices: [2, 3] },
  { id: 'time_digits', label: 'Time Digits', indices: [4, 5, 6, 7] },
  { id: 'home_sets', label: 'Home Sets', indices: [8] },
  { id: 'away_sets', label: 'Away Sets', indices: [9] }
]

const simpleMiscSliders = computed(() => {
  const base = [
    { id: 'cluster_home', label: 'Home LEDs', indices: [LED_HOME_1, LED_HOME_2, LED_HOME_3] },
    { id: 'cluster_away', label: 'Away LEDs', indices: [LED_AWAY_1, LED_AWAY_2, LED_AWAY_3] },
    { id: 'led_mid', label: 'Middle Indicator', indices: [LED_MID] },
    { id: 'time_colons', label: 'Time Colons', indices: [TIME_COLON_TOP, TIME_COLON_BOTTOM] },
  ];
  if (props.isBigBoard) {
    base.push({ id: 'bar_outer', label: 'Outer Bar LEDs', indices: [BAR_LED_1, BAR_LED_4] });
    base.push({ id: 'bar_inner', label: 'Inner Bar LEDs', indices: [BAR_LED_2, BAR_LED_3] });
  } else {
    base.push({ id: 'bar_single', label: 'Bar LED', indices: [BAR_LED_1] });
  }
  return base;
})

const getSimpleDigitValue = (side, slider) => {
  return side === 'A' ? localSegmentA.value[slider.indices[0]] : localSegmentB.value[slider.indices[0]]
}

const updateSimpleDigit = (side, slider, value) => {
  slider.indices.forEach(idx => updateDigit(side, idx, value))
}

const getSimpleMiscValue = (side, slider) => {
  return side === 'A' ? localMiscA.value[slider.indices[0]] : localMiscB.value[slider.indices[0]]
}

const updateSimpleMisc = (side, slider, value) => {
  if (!props.isBigBoard && slider.id === 'bar_inner') value = 0;
  slider.indices.forEach(idx => {
    if (side === 'A') localMiscA.value[idx] = value
    else localMiscB.value[idx] = value
    emit('update:misc', { side, index: idx, value })
  })
}

const isSimpleMiscDisabled = (sliderId) => {
  return false;
}

const activeInnerTab = ref('groups')

const exportTemplate = () => {
  const USED_MISC_INDICES = [10, 11, 12, 13, 14, 15, 16, 20, 21, 22, 23, 24, 25];
  const filteredMiscA = {}
  const filteredMiscB = {}
  USED_MISC_INDICES.forEach(idx => {
    filteredMiscA[idx] = localMiscA.value[idx]
    filteredMiscB[idx] = localMiscB.value[idx]
  })
  
  const data = {
    groupCal: localGroupCal.value,
    segment: activeInnerTab.value === 'sideB' ? localSegmentB.value : localSegmentA.value,
    misc: activeInnerTab.value === 'sideB' ? filteredMiscB : filteredMiscA
  }
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  const dateStr = new Date().toISOString().slice(0, 10)
  a.download = `netscore_calibration_${dateStr}.json`
  a.click()
  URL.revokeObjectURL(url)
}

const fileInput = ref(null)

const triggerImport = () => {
  if (fileInput.value) {
    fileInput.value.click()
  }
}

const handleFileImport = (event) => {
  const file = event.target.files[0]
  if (!file) return

  const reader = new FileReader()
  reader.onload = (e) => {
    try {
      const data = JSON.parse(e.target.result)
      if (data.groupCal) localGroupCal.value = { ...data.groupCal }
      
      if (data.segment) {
        if (activeInnerTab.value === 'sideB') localSegmentB.value = [...data.segment]
        else localSegmentA.value = [...data.segment]
      }
      if (data.misc) {
        let targetMisc = activeInnerTab.value === 'sideB' ? localMiscB : localMiscA
        if (Array.isArray(data.misc)) targetMisc.value = [...data.misc]
        else Object.keys(data.misc).forEach(k => targetMisc.value[k] = data.misc[k])
      }
      
      const fullData = {
        groupCal: localGroupCal.value,
        segmentA: localSegmentA.value,
        segmentB: localSegmentB.value,
        miscA: localMiscA.value,
        miscB: localMiscB.value
      }
      
      emit('import:calibration', fullData)
    } catch (err) {
      console.error('Failed to parse calibration JSON', err)
      alert('Invalid calibration file')
    }
  }
  reader.readAsText(file)
  event.target.value = '' // reset input
}

</script>

<template>
  <n-card size="small" style="background: rgba(0,0,0,0.2);">
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px;">
      <n-switch v-model:value="isAdvancedMode">
        <template #checked>Advanced Mode</template>
        <template #unchecked>Simple Mode</template>
      </n-switch>
      <div style="display: flex; gap: 8px;">
        <n-button size="small" @click="exportTemplate">Export Template</n-button>
        <n-button size="small" type="primary" @click="triggerImport">Import Template</n-button>
        <input type="file" ref="fileInput" accept=".json" style="display: none" @change="handleFileImport" />
      </div>
    </div>
    <n-tabs type="line" animated v-model:value="activeInnerTab">
      <n-tab-pane name="groups" tab="Global Groups">
        <n-space vertical size="large" style="margin-top: 16px;">
          <n-text strong style="color: #ff6b6b;">Red Channels (Big Digits)</n-text>
          <n-slider v-model:value="localGroupCal.r" :min="0" :max="100" @update:value="(v) => updateGroup('r', v)" />
          
          <n-divider style="margin: 4px 0" />
          
          <n-text strong style="color: #63e2b7;">Green Channels (Time)</n-text>
          <n-slider v-model:value="localGroupCal.g" :min="0" :max="100" @update:value="(v) => updateGroup('g', v)" />
          
          <n-divider style="margin: 4px 0" />

          <n-text strong style="color: #70c0e8;">Blue Channels (Sets & Misc)</n-text>
          <n-slider v-model:value="localGroupCal.b" :min="0" :max="100" @update:value="(v) => updateGroup('b', v)" />
        </n-space>
      </n-tab-pane>

      <n-tab-pane name="sideA" tab="Side A">
        <n-grid :cols="2" :x-gap="24" style="margin-top: 16px;" v-if="!isAdvancedMode">
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Segments</n-text>
            <n-space vertical>
              <div v-for="slider in simpleDigitSliders" :key="'SDA'+slider.id">
                <n-text depth="3">{{ slider.label }}</n-text>
                <n-slider :value="getSimpleDigitValue('A', slider)" :min="0" :max="100" @update:value="(v) => updateSimpleDigit('A', slider, v)" />
              </div>
            </n-space>
          </n-grid-item>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Indicator & Bar LEDs</n-text>
            <n-space vertical>
              <div v-for="slider in simpleMiscSliders" :key="'SMA'+slider.id">
                <n-text depth="3">{{ slider.label }}</n-text>
                <n-slider :value="getSimpleMiscValue('A', slider)" :min="0" :max="100" :disabled="isSimpleMiscDisabled(slider.id)" @update:value="(v) => updateSimpleMisc('A', slider, v)" />
              </div>
            </n-space>
          </n-grid-item>
        </n-grid>

        <n-grid :cols="2" :x-gap="24" style="margin-top: 16px;" v-else>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Digits</n-text>
            <n-space vertical>
              <div v-for="(val, index) in localSegmentA" :key="'A'+index">
                <n-text depth="3">{{ getLabel(index) }}</n-text>
                <n-slider v-model:value="localSegmentA[index]" :min="0" :max="100" @update:value="(v) => updateDigit('A', index, v)" />
              </div>
            </n-space>
          </n-grid-item>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Indicator & Bar LEDs</n-text>
            <n-space vertical>
              <div v-for="misc in miscSliders" :key="'MA'+misc.id">
                <n-text depth="3">{{ misc.label }}</n-text>
                <n-slider :value="getMiscValue('A', misc.id)" :min="0" :max="100" :disabled="isSliderDisabled(misc.id)" @update:value="(v) => updateMisc('A', misc.id, v)" />
              </div>
            </n-space>
          </n-grid-item>
        </n-grid>
      </n-tab-pane>

      <n-tab-pane name="sideB" tab="Side B">
        <n-grid :cols="2" :x-gap="24" style="margin-top: 16px;" v-if="!isAdvancedMode">
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Segments</n-text>
            <n-space vertical>
              <div v-for="slider in simpleDigitSliders" :key="'SDB'+slider.id">
                <n-text depth="3">{{ slider.label }}</n-text>
                <n-slider :value="getSimpleDigitValue('B', slider)" :min="0" :max="100" @update:value="(v) => updateSimpleDigit('B', slider, v)" />
              </div>
            </n-space>
          </n-grid-item>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Indicator & Bar LEDs</n-text>
            <n-space vertical>
              <div v-for="slider in simpleMiscSliders" :key="'SMB'+slider.id">
                <n-text depth="3">{{ slider.label }}</n-text>
                <n-slider :value="getSimpleMiscValue('B', slider)" :min="0" :max="100" :disabled="isSimpleMiscDisabled(slider.id)" @update:value="(v) => updateSimpleMisc('B', slider, v)" />
              </div>
            </n-space>
          </n-grid-item>
        </n-grid>

        <n-grid :cols="2" :x-gap="24" style="margin-top: 16px;" v-else>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Digits</n-text>
            <n-space vertical>
              <div v-for="(val, index) in localSegmentB" :key="'B'+index">
                <n-text depth="3">{{ getLabel(index) }}</n-text>
                <n-slider v-model:value="localSegmentB[index]" :min="0" :max="100" @update:value="(v) => updateDigit('B', index, v)" />
              </div>
            </n-space>
          </n-grid-item>
          <n-grid-item>
            <n-text strong style="display:block; margin-bottom: 12px; font-size: 1.1em;">Indicator & Bar LEDs</n-text>
            <n-space vertical>
              <div v-for="misc in miscSliders" :key="'MB'+misc.id">
                <n-text depth="3">{{ misc.label }}</n-text>
                <n-slider :value="getMiscValue('B', misc.id)" :min="0" :max="100" :disabled="isSliderDisabled(misc.id)" @update:value="(v) => updateMisc('B', misc.id, v)" />
              </div>
            </n-space>
          </n-grid-item>
        </n-grid>
      </n-tab-pane>
    </n-tabs>
  </n-card>
</template>
