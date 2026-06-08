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
      const response = await fetch('https://micheline-aeonian-paula.ngrok-free.dev/webhook/69ddccfe-f7be-44b9-adb9-7471dbdc24e4', {
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
        // If it's plain text or not JSON, we keep the original textResponse
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
