# Project Code Annexes

This annex contains the core software implementation for the CatFeeder-T5 system. The source code is organized into three main layers: Embedded Firmware (ESP32/Arduino), Frontend Web Application (React/Vite), and Cloud Backend Integration (Supabase & n8n AI Agent Workflow).

---

## 1. Cloud Backend & AI Integration

The system uses Supabase as a real-time database and authentication provider. High-level cognitive features, such as cat health monitoring and nutritional recommendations, are processed by a serverless n8n agent workflow that connects the database records to a Large Language Model (GPT-4o).

### 1.1. Supabase Client Configuration (`supabase.js`)

This file configures the connection client to the Supabase backend. It enables automatic token refresh and session persistence.

```javascript
import { createClient } from '@supabase/supabase-js'

const supabaseUrl = import.meta.env.VITE_SUPABASE_URL
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY

export const supabase = createClient(supabaseUrl, supabaseAnonKey, {
  auth: {
    persistSession: true,
    autoRefreshToken: true,
  }
})
```

### 1.2. n8n AI Agent Workflow (`My workflow 4.json`)

The n8n workflow exposes a webhook URL called by the front-end chat page. It routes user prompts through an AI Agent equipped with memory buffers and custom tool nodes that run queries against Supabase tables (`cats`, `schedules`, `feed_events`, `devices`, `realtime_weight`).

```json
{
  "name": "My workflow 4",
  "nodes": [
    {
      "parameters": {
        "httpMethod": "POST",
        "path": "69ddccfe-f7be-44b9-adb9-7471dbdc24e4",
        "responseMode": "responseNode",
        "options": {}
      },
      "type": "n8n-nodes-base.webhook",
      "typeVersion": 2.1,
      "position": [
        -160,
        -16
      ],
      "id": "c960ad06-410f-4a6c-a824-0e0ff3ee4cff",
      "name": "Webhook",
      "webhookId": "69ddccfe-f7be-44b9-adb9-7471dbdc24e4"
    },
    {
      "parameters": {
        "promptType": "define",
        "text": "={{ $json.body.chatInput }}",
        "options": {
          "systemMessage": "=Ets VetGat, un assistent especialitzat en salut i nutrició felina connectat a un sistema d'alimentació intel·ligent.\n\n## CAPACITATS\nTens accés en temps real a:\n- El perfil del gat (raça, pes, dieta, edat)\n- L'historial d'àpats (grams consumits, hora, triggers)\n- Els horaris d'alimentació programats\n- El pes actual del menjar al distribuïdor\n- L'estat del dispositiu d'alimentació\n\n## COM ACTUAR\n1. Quan l'usuari mencioni un gat, crida SEMPRE get_cat_profile primer.\n2. Si la pregunta és sobre alimentació, crida get_recent_feed_events \n   i get_feeding_schedules per tenir context real.\n3. Compara els grams objectiu (target_grams) amb els grams reals \n   (actual_grams) per detectar si el gat menja menys del normal.\n4. Si actual_grams < target_grams de forma repetida, avisa que pot \n   ser senyal d'un problema de salut o d'apetit.\n5. Usa date_of_birth per calcular l'edat exacta i adaptar els consells \n   (cadell < 1 any, adult 1-7, sènior > 7).\n6. Si el dispositiu té status != 'online', avisa l'usuari.\n7. Si el hopper té poc menjar (get_hopper_weight baix), informa-ho.\n\n## ALERTES AUTOMÀTIQUES (comprova sempre)\n- Gat no menja > 24h → possible urgència veterinària\n- actual_grams < 50% de target_grams dos àpats seguits → investiga\n- Pèrdua de pes (compara weight_kg amb historial) → recomana visita\n- Dispositiu offline → notifica l'usuari\n\n## RESTRICCIONS\n- No diagnostiques malalties de forma definitiva.\n- No modifiquis horaris ni configuració del dispositiu.\n- No inventis dades: si una tool no retorna res, digues-ho.\n- Si no saps quin gat pregunten, demana el nom.\n\n## TO\nProper, empàtic i professional. Usa 🐾 amb moderació.\nRespon en l'idioma de l'usuari. Cita sempre les dades reals \nque has obtingut (\"Avui en [NOM] ha menjat X grams a les HH:MM\")."
        }
      },
      "type": "@n8n/n8n-nodes-langchain.agent",
      "typeVersion": 3.1,
      "position": [
        64,
        -16
      ],
      "id": "8ee19477-4953-43fd-bc31-51b6ec5adca9",
      "name": "AI Agent"
    },
    {
      "parameters": {
        "model": {
          "__rl": true,
          "value": "gpt-4o",
          "mode": "list",
          "cachedResultName": "gpt-4o"
        },
        "builtInTools": {},
        "options": {}
      },
      "type": "@n8n/n8n-nodes-langchain.lmChatOpenAi",
      "typeVersion": 1.3,
      "position": [
        -80,
        208
      ],
      "id": "510442d1-bb61-4098-8241-09edbbc4ae48",
      "name": "OpenAI Chat Model",
      "credentials": {
        "openAiApi": {
          "id": "5g1S9zdyfwd8BRis",
          "name": "OpenAi account 2"
        }
      }
    },
    {
      "parameters": {
        "sessionIdType": "customKey",
        "sessionKey": "={{ $json.body.sessionId }}",
        "contextWindowLength": 8
      },
      "type": "@n8n/n8n-nodes-langchain.memoryBufferWindow",
      "typeVersion": 1.3,
      "position": [
        80,
        208
      ],
      "id": "b90f7ade-4add-4770-bee3-1d1a049c04f3",
      "name": "Simple Memory"
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Obté el perfil complet d'un gat de la base de dades. \nCrida aquesta tool quan l'usuari mencioni el nom d'un gat o quan \nnecessitis saber l'edat, pes, raça o notes de dieta. \nParàmetre: name (text, nom del gat).",
        "operation": "get",
        "tableId": "cats",
        "filters": {
          "conditions": [
            {
              "keyName": "name",
              "keyValue": "={{ $fromAI('name') }}"
            },
            {
              "keyName": "archived",
              "keyValue": "=false"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        224,
        208
      ],
      "id": "59cc4091-f0ba-4352-b76e-c9f89b527ae8",
      "name": "get_cat_profile",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Obté els horaris d'alimentació programats per a un gat concret.\nMostra a quines hores s'alimenta, quants grams per àpat i quins \ndies de la setmana. Usa'l quan preguntin sobre la rutina d'alimentació.\nParàmetre: cat_id (uuid del gat, obtingut de get_cat_profile).",
        "operation": "get",
        "tableId": "schedules",
        "filters": {
          "conditions": [
            {
              "keyName": "cat_id",
              "keyValue": "={{ $fromAI('cat_id') }}"
            },
            {
              "keyName": "enabled",
              "keyValue": "true"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        368,
        208
      ],
      "id": "c01c5791-4aa4-4433-afa5-fd4ba61dfd8e",
      "name": "get_cat_schedules",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Obté els darrers àpats registrats d'un gat: hora, grams programats \n(target_grams) i grams realment consumits (actual_grams). \nUsa'l per detectar si el gat menja menys del normal, si ha saltat \nàpats o per respondre \"quan ha menjat l'últim cop?\".\nParàmetres: cat_id (uuid), limit (int, per defecte 10).",
        "operation": "get",
        "tableId": "feed_events",
        "filters": {
          "conditions": [
            {
              "keyName": "cat_id",
              "keyValue": "={{ $fromAI('conditions0_Field_Value', ``, 'string') }}"
            },
            {
              "keyName": "status",
              "keyValue": "completed"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        544,
        208
      ],
      "id": "0b582878-1d69-4b0e-9d70-0891eb5ff99f",
      "name": "get_recent_feed_events",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Calcula el total de grams consumits per un gat en els darrers dies, \nagrupat per data. Usa'l per analitzar tendències d'alimentació, \ndetectar dies de poc apetit o respondre preguntes com \n\"ha menjat bé aquesta setmana?\".\nParàmetre: cat_id (uuid).",
        "operation": "getAll",
        "tableId": "feed_events",
        "limit": "={{ /*n8n-auto-generated-fromAI-override*/ $fromAI('Limit', ``, 'number') }}",
        "filters": {
          "conditions": [
            {
              "keyName": "cat_id",
              "condition": "eq",
              "keyValue": "={{ $fromAI('cat_id') }}"
            },
            {
              "keyName": "status",
              "condition": "eq",
              "keyValue": "completed"
            },
            {
              "keyName": "started_at",
              "condition": "gte",
              "keyValue": "={{ DateTime.now().minus({days:7}).toISO() }}"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        224,
        384
      ],
      "id": "93cb61e8-3b88-4686-a500-964b279153be",
      "name": "get_daily_intake_summary",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Comprova l'estat del dispositiu d'alimentació associat a un gat.\nRetorna si el dispositiu és online/offline, quan s'ha vist per última \nvegada i la versió de firmware. Usa'l si l'usuari reporta problemes \namb el dispensador o si vols verificar que el dispositiu funciona.\nParàmetre: cat_id (uuid).",
        "operation": "getAll",
        "tableId": "devices",
        "limit": "={{ /*n8n-auto-generated-fromAI-override*/ $fromAI('Limit', ``, 'number') }}",
        "filters": {
          "conditions": [
            {
              "keyName": "id",
              "condition": "eq",
              "keyValue": "=(SELECT device_id FROM schedules WHERE cat_id = '{{ $fromAI(\"cat_id\") }}' LIMIT 1)"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        400,
        384
      ],
      "id": "4361945d-d538-4bfa-a848-7ce8ad6cd91a",
      "name": "get_device_status",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "descriptionType": "manual",
        "toolDescription": "=Obté el pes actual del menjar dins el distribuïdor (en grams).\nUsa'l per saber si queda menjar al dispensador i avisar l'usuari \nsi cal reomplir-lo. Crida'l quan preguntin \"queda menjar?\" \no quan hagis identificat el device_id del gat.\nParàmetre: device_id (uuid del dispositiu).",
        "operation": "get",
        "tableId": "realtime_weight",
        "filters": {
          "conditions": [
            {
              "keyName": "device_id",
              "keyValue": "={{ $fromAI('device_id') }}"
            }
          ]
        }
      },
      "type": "n8n-nodes-base.supabaseTool",
      "typeVersion": 1,
      "position": [
        560,
        384
      ],
      "id": "e0bf7d78-2b51-49ad-82de-42a2d628ba41",
      "name": "get_hopper_weight",
      "credentials": {
        "supabaseApi": {
          "id": "iy8oqaBXuNv3Bbji",
          "name": "Supabase account 2"
        }
      }
    },
    {
      "parameters": {
        "options": {}
      },
      "type": "n8n-nodes-base.respondToWebhook",
      "typeVersion": 1.5,
      "position": [
        416,
        -16
      ],
      "id": "433e30f7-8c2e-48ef-b8e6-188a1a11223c",
      "name": "Respond to Webhook"
    }
  ],
  "pinData": {},
  "connections": {
    "Webhook": {
      "main": [
        [
          {
            "node": "AI Agent",
            "type": "main",
            "index": 0
          }
        ]
      ]
    },
    "OpenAI Chat Model": {
      "ai_languageModel": [
        [
          {
            "node": "AI Agent",
            "type": "ai_languageModel",
            "index": 0
          }
        ]
      ]
    },
    "Simple Memory": {
      "ai_memory": [
        [
          {
            "node": "AI Agent",
            "type": "ai_memory",
            "index": 0
          }
        ]
      ]
    },
    "get_cat_profile": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "get_cat_schedules": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "get_recent_feed_events": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "get_daily_intake_summary": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "get_device_status": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "get_hopper_weight": {
      "ai_tool": [
        [
          {
            "node": "AI Agent",
            "type": "ai_tool",
            "index": 0
          }
        ]
      ]
    },
    "AI Agent": {
      "main": [
        [
          {
            "node": "Respond to Webhook",
            "type": "main",
            "index": 0
          }
        ]
      ]
    },
    "Respond to Webhook": {
      "main": [
        []
      ]
    }
  },
  "active": true,
  "settings": {
    "executionOrder": "v1",
    "binaryMode": "separate",
    "availableInMCP": false
  },
  "versionId": "952d3416-e287-4741-9818-f4020702c041",
  "meta": {
    "templateCredsSetupCompleted": true,
    "instanceId": "72ca13be64d15e8dd81551769c8c432039a52625acd9c2df019b7b7221f27d72"
  },
  "id": "0bZu5vRBINVgbBIm",
  "tags": []
}
```

---

## 2. Frontend Web Application

The user-facing client is a single page application built on React, Vite, and Zustand for state management. It communicates with Supabase in real-time, displaying active dispensing events and embedding the health AI chat panel.

### 2.1. Global Zustand Store (`store.js`)

Manages client-side cache and encapsulates Supabase database queries.

```javascript
import { create } from 'zustand'
import { supabase } from '../lib/supabase'

export const useAuthStore = create((set) => ({
  user: null,
  session: null,
  loading: true,
  setUser: (user) => set({ user }),
  setSession: (session) => set({ session, user: session?.user ?? null }),
  setLoading: (loading) => set({ loading }),
  signOut: async () => {
    await supabase.auth.signOut()
    set({ user: null, session: null })
  }
}))

export const useAppStore = create((set, get) => ({
  cats: [],
  devices: [],
  schedules: [],
  notifications: [],
  toasts: [],
  activeFeed: null,

  setCats: (cats) => set({ cats }),
  setDevices: (devices) => set({ devices }),
  setSchedules: (schedules) => set({ schedules }),
  setNotifications: (notifications) => set({ notifications }),
  setActiveFeed: (activeFeed) => set({ activeFeed }),

  addToast: (message, type = 'info') => {
    const id = Date.now()
    set((s) => ({ toasts: [...s.toasts, { id, message, type }] }))
    setTimeout(() => {
      set((s) => ({ toasts: s.toasts.filter(t => t.id !== id) }))
    }, 3500)
  },
  removeToast: (id) => set((s) => ({ toasts: s.toasts.filter(t => t.id !== id) })),

  fetchCats: async () => {
    const { data } = await supabase.from('cats').select('*').eq('archived', false).order('created_at')
    if (data) set({ cats: data })
  },
  fetchDevices: async () => {
    const { data } = await supabase.from('devices').select('*').order('created_at')
    if (data) set({ devices: data })
  },
  fetchSchedules: async () => {
    const { data } = await supabase.from('schedules').select('*, cats(name)').order('time_of_day')
    if (data) set({ schedules: data })
  },
  fetchNotifications: async () => {
    const { data } = await supabase.from('notifications').select('*').eq('read', false).order('created_at', { ascending: false }).limit(20)
    if (data) set({ notifications: data })
  },
}))
```

### 2.2. Conversational AI Chat Component (`ChatPage.jsx`)

Implements the user interface for sending messages to the n8n webhook and rendering responses from the VetGat AI.

```javascript
import { useState, useRef, useEffect } from 'react'
import { useAuthStore } from '../store/store'
import './ChatPage.css'

export default function ChatPage() {
  const { user } = useAuthStore()
  const [messages, setMessages] = useState([
    { id: 'initial', role: 'bot', text: 'Hi there! I am the CatFeeder AI assistant.\nAsk me anything about your cats in the database.' }
  ])
  const [input, setInput] = useState('')
  const [isLoading, setIsLoading] = useState(false)
  const messagesEndRef = useRef(null)

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }

  useEffect(() => {
    scrollToBottom()
  }, [messages, isLoading])

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!input.trim() || isLoading) return

    const userMessage = {
      id: Date.now().toString(),
      role: 'user',
      text: input.trim()
    }

    setMessages(prev => [...prev, userMessage])
    setInput('')
    setIsLoading(true)

    try {
      const response = await fetch('http://192.168.1.42:5678/webhook/69ddccfe-f7be-44b9-adb9-7471dbdc24e4', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json'
        },
        body: JSON.stringify({
          sessionId: user?.id || 'anonymous',
          chatInput: userMessage.text
        })
      })

      if (!response.ok) throw new Error('Network response was not ok')
      
      const textResponse = await response.text();
      let botText = textResponse;
      
      try {
        const jsonData = JSON.parse(textResponse);
        if (Array.isArray(jsonData) && jsonData.length > 0) {
            botText = jsonData[0].output || jsonData[0].text || jsonData[0].message || JSON.stringify(jsonData[0]);
        } else if (jsonData && typeof jsonData === 'object') {
            botText = jsonData.output || jsonData.text || jsonData.message || jsonData.response || JSON.stringify(jsonData);
        }
      } catch (e) {
      }

      setMessages(prev => [...prev, {
        id: Date.now().toString() + 'bot',
        role: 'bot',
        text: botText || 'Received empty response from the bot.'
      }])

    } catch (error) {
      console.error("Chat error:", error)
      setMessages(prev => [...prev, {
        id: Date.now().toString() + 'err',
        role: 'bot',
        text: 'Sorry, I failed to connect to the webhook. Please try again.'
      }])
    } finally {
      setIsLoading(false)
    }
  }

  return (
    <div className="page-enter chat-page">
      <div className="page-header" style={{ marginBottom: '16px' }}>
        <div>
          <h1>CatFeeder Agent</h1>
          <div className="subtitle">Chat with our AI to analyze and get insights about your cats.</div>
        </div>
      </div>

      <div className="chat-messages">
        {messages.map((msg) => (
          <div key={msg.id} className={`message ${msg.role}`}>
            <div className="message-bubble">
              {msg.text}
            </div>
          </div>
        ))}
        {isLoading && (
          <div className="message bot">
            <div className="message-bubble" style={{ padding: '16px' }}>
              <div className="typing-indicator">
                <span></span><span></span><span></span>
              </div>
            </div>
          </div>
        )}
        <div ref={messagesEndRef} />
      </div>

      <form className="chat-input-form" onSubmit={handleSubmit}>
        <input
          type="text"
          className="form-input"
          placeholder="Ask something about your cats..."
          value={input}
          onChange={(e) => setInput(e.target.value)}
          disabled={isLoading}
          autoFocus
        />
        <button type="submit" className="btn btn-primary" disabled={isLoading || !input.trim()}>
          {isLoading ? '...' : 'Send'}
        </button>
      </form>
    </div>
  )
}
```

### 2.3. Dispenser Trigger Component (`FeedNowModal.jsx`)

Provides the interface logic to write manual feeding commands to the Supabase database.

```javascript
import { useState } from 'react'
import { supabase } from '../lib/supabase'

export default function FeedNowModal({ cats, device, onClose, onFed }) {
  const [catId, setCatId] = useState(cats[0]?.id ?? '')
  const [portion, setPortion] = useState(50)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const handleFeed = async () => {
    if (!device) { setError('No device registered. Go to Device page first.'); return }
    if (!catId) { setError('Select a cat first.'); return }
    setLoading(true); setError('')
    try {
      const { data, error: err } = await supabase.from('commands').insert({
        device_id: device.id,
        cat_id: catId,
        command_type: 'feed',
        portion_grams: portion,
        status: 'pending'
      }).select().single()
      if (err) throw err
      onFed(data)
    } catch (e) {
      setError(e.message)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="modal-overlay" onClick={e => { if (e.target === e.currentTarget) onClose() }}>
      <div className="modal">
        <div className="modal-header">
          <h3>⚡ Feed Now</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        <div className="modal-body">
          {error && <div className="auth-error">{error}</div>}
          <div className="form-group">
            <label className="form-label">Select Cat</label>
            <select className="form-select" value={catId} onChange={e => setCatId(e.target.value)}>
              {cats.map(c => <option key={c.id} value={c.id}>{c.name}</option>)}
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Portion Size: <strong>{portion}g</strong></label>
            <input
              type="range"
              min={5} max={500} step={5}
              value={portion}
              onChange={e => setPortion(Number(e.target.value))}
              style={{width:'100%',accentColor:'var(--accent)'}}
            />
            <div style={{display:'flex',justifyContent:'space-between',fontSize:'0.75rem',color:'var(--text-faint)'}}>
              <span>5g</span><span>500g</span>
            </div>
          </div>
          <div className="card" style={{padding:'14px 16px'}}>
            <div style={{fontSize:'0.82rem',color:'var(--text-muted)',marginBottom:4}}>Feed summary</div>
            <div style={{fontFamily:'var(--font-display)',fontSize:'1.1rem'}}>
              {cats.find(c=>c.id===catId)?.name ?? '—'} → <strong style={{color:'var(--accent)'}}>{portion}g</strong>
            </div>
          </div>
        </div>
        <div className="modal-footer">
          <button className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button id="confirm-feed-btn" className="btn btn-primary" onClick={handleFeed} disabled={loading}>
            {loading ? '◌ Sending…' : '⚡ Confirm Feed'}
          </button>
        </div>
      </div>
    </div>
  )
}
```

---

## 3. Embedded Firmware (ESP32 / Arduino)

The physical device uses an ESP32 micro-controller that links a load cell (HX711) and a stepper motor driver (DRV8825) with Supabase REST APIs.

### 3.1. Main System Header (`config.h`)

Defines all global parameters, GPIO pins, data structures, and peripheral setups.

```cpp
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

#define FW_VERSION "0.2.0-touch"
#define SERIAL_BAUD 115200

#define WIFI_SSID "Lab-Modul"
#define WIFI_PASSWORD "GVe836Nf"
#define WIFI_RECONNECT_MS 10000
#define WIFI_CONNECT_TIMEOUT_MS 15000

#define SUPABASE_URL "https://jawqxuzlvvzsrobftupx.supabase.co"
#define SUPABASE_ANON_KEY                                                      \
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."                                      \
  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imphd3F4dXpsdnZ6c3JvYmZ0dXB4Iiwicm9sZSI6Im" \
  "Fub24iLCJpYXQiOjE3NzM4NjY2MjcsImV4cCI6MjA4OTQ0MjYyN30."                     \
  "FnvhOpqhZh9Z3j2XkIPla1wUbx3wAsaP4anr44Utrzs"
#define DEVICE_ID "44b0f051-549a-4b87-b67a-592254e5c84f"

#define DISPLAY_ROTATION 1
#define TOUCH_CS 21

#define I2C_SDA 32
#define I2C_SCL 33
#define I2C_CLOCK_HZ 400000

#define HX711_DT 16
#define HX711_SCK 17
#define HX711_GAIN 128
#define HX711_SAMPLES 5
#define HX711_CALIBRATION_FACTOR -2027.0f
#define SCALE_STABLE_THRESHOLD_G 0.3f

#define DHT_PIN 14
#define DHT_READ_INTERVAL_MS 2500

#define STEPPER_STEP 26
#define STEPPER_DIR 27
#define STEPPER_EN 25
#define STEPPER_STEPS_PER_REV 200
#define STEPPER_MICROSTEPS 16
#define STEPPER_MAX_SPEED 2000.0f
#define STEPPER_ACCEL 1500.0f
#define STEPPER_DISPENSE_SPEED 1200.0f
#define STEPPER_MAX_RUN_MS 30000
#define STEPPER_FEED_RATE_GPS 8.0f

#define DEFAULT_PORTION_G 25.0f
#define MIN_PORTION_G 5.0f
#define MAX_PORTION_G 500.0f
#define FEED_TOLERANCE_G 1.5f
#define FEED_STOP_THRESHOLD 0.90f
#define FEED_SETTLE_MS 500

#define TELEMETRY_INTERVAL_MS 2000
#define HEARTBEAT_INTERVAL_MS 30000
#define COMMAND_POLL_INTERVAL_MS 5000
#define CONFIG_POLL_INTERVAL_MS 60000
#define REALTIME_WEIGHT_INTERVAL_MS 500
#define DISPLAY_REFRESH_MS 250

enum DeviceState {
  STATE_BOOT,
  STATE_IDLE,
  STATE_DISPENSING,
  STATE_ERROR,
  STATE_OFFLINE_DEGRADED
};
extern DeviceState currentState;
extern const char *lastErrorMessage;

struct Telemetry {
  float weightG;
  float temperatureC;
  float humidity;
  bool wifiUp;
};
extern Telemetry telemetry;

struct FeedingCycle {
  bool active;
  float targetG;
  float dispensedG;
  float baselineG;
  uint32_t startMs;
  uint32_t lastPublishMs;
  const char *trigger;
  String commandId;
  String catId;
  String scheduleId;
  String startedAtIso;
};
extern FeedingCycle cycle;

enum UIMode {
  UI_MODE_MANUAL,
  UI_MODE_AUTO
};
extern UIMode uiRequestedMode;

enum UIScreen {
  UI_SCREEN_SCREENSAVER,
  UI_SCREEN_UNLOCK,
  UI_SCREEN_MAIN,
  UI_SCREEN_MANUAL_FEED,
  UI_SCREEN_AUTO_MODE
};
extern UIScreen activeUIScreen;
extern bool screenDirty;
extern uint8_t scheduleCount;

extern bool eyesOpen;
extern bool cornerHit[4];
extern uint8_t cornerHitCount;

extern uint32_t lastTouchMs;
extern uint32_t lastBlinkMs;
extern uint32_t lastPeriodicRedrawMs;
extern uint32_t lastManualDynamicMs;
extern uint32_t firstCornerMs;

#define UI_COL_BG               0x0820
#define UI_COL_CARD             0x1082
#define UI_COL_ACCENT           0x07FF
#define UI_COL_OK               0x07E0
#define UI_COL_DANGER           0xF800
#define UI_COL_WARN             0xFD20
#define UI_COL_TEXT             0xFFFF
#define UI_COL_MUTED            0x7BCF
#define UI_COL_OVERLAY          0x0000
#define UI_COL_CAT_LINE         0xC618

#define UI_IDLE_TIMEOUT_MS      30000
#define CAT_BLINK_INTERVAL_MS   3000
#define CORNER_HIT_ZONE_PX      56
#define CORNER_TIMEOUT_MS       5000
#define TOUCH_Z_THRESHOLD       350

#define MAIN_REDRAW_MS          1000
#define AUTO_REDRAW_MS          1000
#define MANUAL_DYNAMIC_MS       250

#define MANUAL_BTN_X            18
#define MANUAL_BTN_Y            146
#define MANUAL_BTN_W            284
#define MANUAL_BTN_H            74

#define MANUAL_BACK_W           96
#define HEADER_H                32
#define FOOTER_H                18

void touchUIInit();
void touchInputInit();
void touchInputUpdate();
void resetUnlockPattern();

void displayInit();
void displaySplash(const char *text);
void displayUpdate();
void transitionTo(UIScreen next);

void drawScreensaver();
void drawUnlockScreen();
void drawMainMenu();
void drawManualFeedScreen();
void drawManualFeedDynamic(bool force);
void drawAutoModeScreen();

void scaleInit();
void scaleTare();
float scaleRead();
void scaleCalibrate(float knownWeightG);

bool startDispense(float grams, const char *trigger, const String &commandId, const String &catId, const String &scheduleId);
void stopDispense();
void motorEmergencyStop();
void runDispensingCycle();
void checkScheduledFeeds();

struct DeviceTime {
  bool valid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t wday;
  uint16_t yday;
};
extern DeviceTime currentDeviceTime;

#define TIMEZONE_OFFSET_SEC 7200
void updateTimeFromSupabase(const char* utcStr);
void updateLocalClock();

void sendHeartbeat();
void pollPendingCommands();
void pollDeviceConfig();
void publishRealtimeWeight(float weightG, float dispensedG, float targetG);
void logFeedEvent(const char *catId, float targetG, float actualG, const char *trigger, const char *status);
void updateCommandStatus(const String &cmdId, const char *status, float actualGrams);

#endif
```

### 3.2. Shared Scheduling Data Structure (`schedules.h`)

Defines the structure used to program feeding routines locally in the micro-controller's memory.

```cpp
#ifndef SCHEDULES_H
#define SCHEDULES_H

#include <Arduino.h>

#define MAX_SCHEDULES 20

struct Schedule {
  bool     enabled;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  daysMask;
  float    portionG;
  String   catId;
  String   id;
  uint16_t lastFiredYday;
};

#endif
```

### 3.3. TFT Display SPI Pin Definition (`User_Setup.h`)

Maps the ILI9341 display pins to the shared SPI bus lines of the ESP32.

```cpp
#define USER_SETUP_INFO "ESP32_ILI9341_TOUCH"

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18

#define TFT_CS   5
#define TFT_DC   22
#define TFT_RST  -1

#define TOUCH_CS 21

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4

#define SPI_FREQUENCY       10000000
#define SPI_READ_FREQUENCY  6000000
#define SPI_TOUCH_FREQUENCY 1000000
```

### 3.4. System Core Loop and Scheduler (`CatFeeder.ino`)

Coordinated orchestrator that updates UI displays, checks schedules, executes non-blocking state loops, and runs HTTP synchronization handlers.

```cpp
#include "config.h"

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HX711.h>
#include <TFT_eSPI.h>

#include "schedules.h"

TFT_eSPI tft = TFT_eSPI();
HX711 scale;
DHT dht(DHT_PIN, DHT22);
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

DeviceState currentState = STATE_BOOT;
const char *lastErrorMessage = "";

Telemetry telemetry = {0.0f, NAN, NAN, false};

FeedingCycle cycle = {false, 0.0f, 0.0f, 0.0f, 0, 0, "manual", "", "", "", ""};

extern uint8_t  scheduleCount;

static uint32_t tLastSensors      = 0;
static uint32_t tLastDisplay      = 0;
static uint32_t tLastHeartbeat    = 0;
static uint32_t tLastCmdPoll      = 0;
static uint32_t tLastCfgPoll      = 0;
static uint32_t tLastWifiTry      = 0;
static uint32_t tLastWeightPublish = 0;
#define IDLE_WEIGHT_PUBLISH_MS 10000

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(150);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  CatFeeder — firmware " FW_VERSION));
  Serial.println(F("========================================"));

  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, HIGH);

  touchInputInit();

  displayInit();
  displaySplash("Booting...");

  i2cInit();
  scaleInit();
  sensorsInit();
  motorInit();
  networkInit();

  touchUIInit();

  currentState = STATE_IDLE;
  displaySplash("Ready");
  delay(400);

  Serial.println(F("[boot] complete"));
}

void loop() {
  const uint32_t now = millis();

  static uint32_t tLastTouchUpdate = 0;
  if (now - tLastTouchUpdate >= 60) {
    tLastTouchUpdate = now;
    touchInputUpdate();
  }

  switch (currentState) {
  case STATE_IDLE:
    if (uiRequestedMode == UI_MODE_AUTO) {
      checkScheduledFeeds();
    }
    break;
  case STATE_DISPENSING:
    runDispensingCycle();
    if (now - cycle.lastPublishMs >= REALTIME_WEIGHT_INTERVAL_MS) {
      cycle.lastPublishMs = now;
      if (telemetry.wifiUp) {
        publishRealtimeWeight(telemetry.weightG, cycle.dispensedG, cycle.targetG);
      }
    }
    break;
  case STATE_ERROR:
    break;
  default:
    break;
  }

  if (now - tLastSensors >= TELEMETRY_INTERVAL_MS) {
    tLastSensors = now;
    updateSensors();
  }

  if (now - tLastDisplay >= DISPLAY_REFRESH_MS) {
    tLastDisplay = now;
    displayUpdate();
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!telemetry.wifiUp) {
      telemetry.wifiUp = true;
      tLastHeartbeat = now;
      sendHeartbeat();
      tLastCfgPoll = now;
      pollDeviceConfig();
      tLastCmdPoll = now;
      pollPendingCommands();
    }
    if (now - tLastCmdPoll >= COMMAND_POLL_INTERVAL_MS) {
      tLastCmdPoll = now;
      pollPendingCommands();
    }
    if (now - tLastCfgPoll >= CONFIG_POLL_INTERVAL_MS) {
      tLastCfgPoll = now;
      pollDeviceConfig();
    }
    if (now - tLastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
      tLastHeartbeat = now;
      sendHeartbeat();
    }
    if (!cycle.active && now - tLastWeightPublish >= IDLE_WEIGHT_PUBLISH_MS) {
      tLastWeightPublish = now;
      publishRealtimeWeight(telemetry.weightG, 0, 0);
    }
  } else {
    telemetry.wifiUp = false;
    if (now - tLastWifiTry >= WIFI_RECONNECT_MS) {
      tLastWifiTry = now;
      networkReconnect();
    }
  }
}

void i2cInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  Serial.println(F("[i2c] started"));
}

void goToError(const char *message) {
  lastErrorMessage = message;
  currentState = STATE_ERROR;
  motorEmergencyStop();
  Serial.print(F("[ERROR] "));
  Serial.println(message);
}

void clearError() {
  lastErrorMessage = "";
  currentState = STATE_IDLE;
  Serial.println(F("[ERROR] cleared"));
}

void cycleFinish(const char *status, float dispensed) {
  cycle.active = false;
  cycle.dispensedG = dispensed;
  motorEnable(false);

  if (telemetry.wifiUp) {
    logFeedEvent(cycle.catId.c_str(), cycle.targetG, dispensed,
                 cycle.trigger, status);
    if (cycle.commandId.length() > 0) {
      updateCommandStatus(cycle.commandId, status, dispensed);
      cycle.commandId = "";
    }
  }

  currentState = STATE_IDLE;
  Serial.printf("[cycle] %s — %.1fg dispensed\n", status, dispensed);
}
```

### 3.5. REST Client Database Link (`Network.ino`)

Establishes connection headers, executes API requests for telemetry publishing, claims incoming commands, logs finished feed events, and queries schedules from Supabase REST endpoints.

```cpp
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static WiFiClientSecure netClient;

void loadSchedules(JsonArray &arr);
extern uint8_t scheduleCount;
void scaleTare();
float scaleRead();

static const char *HDR_APIKEY = "apikey";
static const char *HDR_AUTH = "Authorization";

void networkInit() {
  netClient.setInsecure();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[wifi] connecting to %s ", WIFI_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    Serial.print('.');
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] ok, IP=%s\n", WiFi.localIP().toString().c_str());
    telemetry.wifiUp = true;
  } else {
    Serial.println(F("[wifi] failed — continuing offline"));
    telemetry.wifiUp = false;
  }
}

void networkReconnect() {
  if (WiFi.status() == WL_CONNECTED)
    return;
  Serial.println(F("[wifi] reconnecting..."));
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void applySupabaseHeaders(HTTPClient &http, bool patch = false) {
  http.addHeader(HDR_APIKEY, SUPABASE_ANON_KEY);
  http.addHeader(HDR_AUTH, String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  if (patch)
    http.addHeader("Prefer", "return=minimal");
}

static int httpGET(const String &endpoint, String &out) {
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http);
  int code = http.GET();
  if (code > 0)
    out = http.getString();
  http.end();
  return code;
}

static int httpPOST(const String &endpoint, const String &body) {
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http);
  int code = http.POST(body);
  http.end();
  return code;
}

static int httpPATCH(const String &endpoint, const String &body) {
  if (WiFi.status() != WL_CONNECTED)
    return -1;
  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + endpoint);
  applySupabaseHeaders(http, true);
  int code = http.PATCH(body);
  http.end();
  return code;
}

static String isoTimestamp() {
  return String("now");
}

void sendHeartbeat() {
  JsonDocument doc;
  doc["last_seen"] = "now";
  doc["status"] = cycle.active
                      ? "dispensing"
                      : (currentState == STATE_ERROR ? "fault_motor" : "idle");
  doc["firmware_version"] = FW_VERSION;
  String body;
  serializeJson(doc, body);

  if (WiFi.status() != WL_CONNECTED)
    return;

  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + "/rest/v1/devices?id=eq." DEVICE_ID);
  
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");
  
  int code = http.PATCH(body);
  Serial.printf("[net] heartbeat HTTP %d\n", code);
  
  if (code == 200 || code == 201) {
    String payload = http.getString();
    JsonDocument respDoc;
    DeserializationError err = deserializeJson(respDoc, payload);
    if (!err && respDoc.is<JsonArray>() && respDoc.size() > 0) {
      const char* lastSeen = respDoc[0]["last_seen"] | "";
      if (lastSeen && lastSeen[0]) {
        updateTimeFromSupabase(lastSeen);
      }
    }
  }
  http.end();
}

void publishRealtimeWeight(float weightG, float dispensedG, float targetG) {
  if (WiFi.status() != WL_CONNECTED)
    return;
  JsonDocument doc;
  doc["device_id"]    = DEVICE_ID;
  doc["weight_grams"] = weightG;
  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(netClient, String(SUPABASE_URL) + "/rest/v1/realtime_weight");
  applySupabaseHeaders(http);
  http.addHeader("Prefer", "resolution=merge-duplicates");
  int code = http.POST(body);
  http.end();
  Serial.printf("[net] realtime_weight HTTP %d (%.1fg)\n", code, weightG);
}

void logFeedEvent(const char *catId, float targetG, float actualG,
                  const char *trigger, const char *status) {
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  if (catId && catId[0])
    doc["cat_id"] = catId;
  if (cycle.scheduleId.length())
    doc["schedule_id"] = cycle.scheduleId;
  doc["target_grams"] = targetG;
  doc["actual_grams"] = actualG;
  doc["trigger_type"] = trigger;
  doc["status"] = status;
  doc["started_at"] = cycle.startedAtIso;

  String body;
  serializeJson(doc, body);
  int code = httpPOST("/rest/v1/feed_events", body);
  if (code < 200 || code >= 300) {
    Serial.printf("[net] feed_event HTTP %d\n", code);
  }
}

void updateCommandStatus(const String &cmdId, const char *status,
                         float actualGrams) {
  JsonDocument doc;
  doc["status"] = status;
  String body;
  serializeJson(doc, body);
  String ep = "/rest/v1/commands?id=eq." + cmdId;
  httpPATCH(ep, body);
}

void pollPendingCommands() {
  String ep = "/rest/v1/commands?device_id=eq." DEVICE_ID
              "&status=eq.pending&select=id,cat_id,portion_grams,command_type"
              "&limit=1";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200) {
    Serial.printf("[net] cmd poll HTTP %d\n", code);
    return;
  }
  if (payload.length() < 3) {
    Serial.println(F("[net] cmd poll: none pending"));
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err || !doc.is<JsonArray>() || doc.size() == 0)
    return;

  if (currentState != STATE_IDLE)
    return;

  JsonObject cmd = doc[0];
  String id = cmd["id"] | "";
  String cat = cmd["cat_id"] | "";
  float grams = cmd["portion_grams"] | DEFAULT_PORTION_G;
  String type = cmd["command_type"] | "dispense";

  if (id.length() == 0)
    return;

  Serial.printf("[net] command %s, type=%s, %.1f g\n", id.c_str(), type.c_str(), grams);

  {
    JsonDocument up;
    up["status"] = "processing";
    String body;
    serializeJson(up, body);
    httpPATCH("/rest/v1/commands?id=eq." + id, body);
  }

  if (type == "tare") {
    scaleTare();
    updateCommandStatus(id, "completed", 0.0f);
    telemetry.weightG = scaleRead();
    publishRealtimeWeight(telemetry.weightG, 0, 0);
    return;
  }

  if (!startDispense(grams, "manual", id, cat, String(""))) {
    updateCommandStatus(id, "error", 0.0f);
  }
}

void pollDeviceConfig() {
  String ep = "/rest/v1/schedules?enabled=eq.true"
              "&select=id,cat_id,time_of_day,days_of_week,portion_grams";
  String payload;
  int code = httpGET(ep, payload);
  if (code != 200 || payload.length() < 2)
    return;

  JsonDocument doc;
  if (deserializeJson(doc, payload))
    return;
  if (!doc.is<JsonArray>())
    return;

  JsonArray arr = doc.as<JsonArray>();
  loadSchedules(arr);
  Serial.printf("[net] schedules loaded: %u\n", scheduleCount);
}
```

### 3.6. Non-Blocking Motor Control (`Motor.ino`)

Implements step generation via hardware PWM (LEDC), applying a 90% gravimetric early cutoff threshold to account for in-flight food and a 500 ms post-dispense settling filter.

```cpp
#define MOTOR_STEP_HZ       500
#define MOTOR_LEDC_BITS     8
#define MOTOR_DUTY_RUN      128
#define MOTOR_DUTY_STOP     0

#define DISPENSE_SCALE_MS   300UL
static uint32_t lastScaleCheckMs = 0;

static bool     waitingFinalRead = false;
static uint32_t settleStartMs    = 0;

void motorInit() {
  pinMode(STEPPER_EN,  OUTPUT);
  pinMode(STEPPER_DIR, OUTPUT);
  digitalWrite(STEPPER_EN,  HIGH);
  digitalWrite(STEPPER_DIR, LOW);

  ledcAttach(STEPPER_STEP, MOTOR_STEP_HZ, MOTOR_LEDC_BITS);
  ledcWrite(STEPPER_STEP, MOTOR_DUTY_STOP);

  Serial.println(F("[motor] stepper driver ready (LEDC hardware PWM)"));
}

void motorEnable(bool on) {
  digitalWrite(STEPPER_EN, on ? LOW : HIGH);
}

void motorEmergencyStop() {
  ledcWrite(STEPPER_STEP, MOTOR_DUTY_STOP);
  motorEnable(false);
  cycle.active     = false;
  waitingFinalRead = false;
  Serial.println(F("[motor] emergency stop"));
}

void stopDispense() {
  if (currentState == STATE_DISPENSING && cycle.active && !waitingFinalRead) {
    ledcWrite(STEPPER_STEP, MOTOR_DUTY_STOP);
    motorEnable(false);
    waitingFinalRead = true;
    settleStartMs    = millis();
    Serial.printf("[motor] manual dispense stopped — settling %ums\n", (unsigned)FEED_SETTLE_MS);
  }
}

bool startDispense(float grams,
                   const char *trigger,
                   const String &commandId,
                   const String &catId,
                   const String &scheduleId) {
  if (currentState == STATE_DISPENSING || cycle.active) {
    Serial.println(F("[motor] rejected: already dispensing"));
    return false;
  }
  if (grams < MIN_PORTION_G || grams > MAX_PORTION_G) {
    Serial.printf("[motor] rejected: %.1f g out of range\n", grams);
    return false;
  }

  cycle.active        = true;
  cycle.targetG       = grams;
  cycle.dispensedG    = 0.0f;
  cycle.baselineG     = scaleRead();
  Serial.printf("[motor] baseline=%.1fg\n", cycle.baselineG);
  cycle.startMs       = millis();
  cycle.lastPublishMs = 0;
  cycle.trigger       = trigger;
  cycle.commandId     = commandId;
  cycle.catId         = catId;
  cycle.scheduleId    = scheduleId;
  cycle.startedAtIso  = isoTimestamp();

  waitingFinalRead = false;
  lastScaleCheckMs = millis();

  digitalWrite(STEPPER_DIR, LOW);
  motorEnable(true);
  delay(5);

  ledcWrite(STEPPER_STEP, MOTOR_DUTY_RUN);

  currentState = STATE_DISPENSING;
  Serial.printf("[motor] dispensing %.1fg (%s) — LEDC @%dHz, stop@%.0f%%\n",
                grams, trigger, MOTOR_STEP_HZ,
                FEED_STOP_THRESHOLD * 100.0f);
  return true;
}

void runDispensingCycle() {
  const uint32_t nowMs = millis();

  if (waitingFinalRead) {
    if (nowMs - settleStartMs >= FEED_SETTLE_MS) {
      float raw = cycle.baselineG + cycle.dispensedG;
      uint32_t tStart = millis();
      bool success = false;
      while (millis() - tStart < 150) {
        if (scale.is_ready()) {
          raw = scale.get_units(3);
          success = true;
          break;
        }
        delay(10);
      }
      float finalDispensed = raw - cycle.baselineG;
      if (finalDispensed < 0) finalDispensed = 0.0f;
      if (success) {
        telemetry.weightG = raw;
      }
      waitingFinalRead = false;
      cycleFinish("completed", finalDispensed);
    }
    return;
  }

  if (nowMs - lastScaleCheckMs >= DISPENSE_SCALE_MS && scale.is_ready()) {
    lastScaleCheckMs = nowMs;
    float raw  = scale.get_units(1);
    float disp = raw - cycle.baselineG;
    if (disp < 0) disp = 0;
    cycle.dispensedG = disp;
    telemetry.weightG = raw;
  }

  const bool scaleReliable = (nowMs - cycle.startMs) >= 2000UL;

  if (scaleReliable && cycle.dispensedG >= cycle.targetG * FEED_STOP_THRESHOLD) {
    ledcWrite(STEPPER_STEP, MOTOR_DUTY_STOP);
    motorEnable(false);
    waitingFinalRead = true;
    settleStartMs    = nowMs;
    Serial.printf("[motor] reached %.0f%% (%.1f/%.1fg) — settling %ums\n",
                  FEED_STOP_THRESHOLD * 100.0f, cycle.dispensedG,
                  cycle.targetG, (unsigned)FEED_SETTLE_MS);
    return;
  }

  if (nowMs - cycle.startMs > STEPPER_MAX_RUN_MS) {
    ledcWrite(STEPPER_STEP, MOTOR_DUTY_STOP);
    motorEmergencyStop();
    if (cycle.dispensedG > 0.5f) {
      cycleFinish("partial", cycle.dispensedG);
    } else {
      cycleFinish("error", 0.0f);
      goToError("Motor timeout");
    }
  }
}
```

### 3.7. Load Cell Weight Sensing (`Scale.ino`)

Integrates with the HX711 24-bit ADC, averaging samples and filtering noise below a stable scale threshold.

```cpp
void scaleInit() {
  scale.begin(HX711_DT, HX711_SCK);

  uint32_t t0 = millis();
  while (!scale.is_ready() && millis() - t0 < 1500) {
    delay(50);
  }

  if (!scale.is_ready()) {
    Serial.println(F("[scale] HX711 not responding — check wiring (continuing)"));
    return;
  }

  scale.set_scale(HX711_CALIBRATION_FACTOR);
  scale.tare(HX711_SAMPLES);
  Serial.printf("[scale] ready, factor=%.2f\n", HX711_CALIBRATION_FACTOR);
}

void scaleTare() {
  if (cycle.active) {
    Serial.println(F("[scale] tare rejected: cycle active"));
    return;
  }
  scale.tare(HX711_SAMPLES);
  Serial.println(F("[scale] tare done"));
}

float scaleRead() {
  if (!scale.is_ready()) return telemetry.weightG;
  float g = scale.get_units(HX711_SAMPLES);
  if (fabsf(g) < SCALE_STABLE_THRESHOLD_G) g = 0.0f;
  return g;
}

void scaleCalibrate(float knownWeightG) {
  Serial.println(F("[scale] calibrating..."));
  scale.set_scale();
  delay(500);
  long raw = scale.read_average(20);
  float newFactor = (float)raw / knownWeightG;
  scale.set_scale(newFactor);
  Serial.printf("[scale] new calibration factor = %.2f\n", newFactor);
  Serial.println(F("[scale] update HX711_CALIBRATION_FACTOR in config.h"));
}
```
