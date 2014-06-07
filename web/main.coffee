
class Vector3
  constructor: (@x, @y, @z) -> return


class Quaternion
  constructor: (@w, @x, @y, @z) -> return
  multiply: (o) ->
    new Quaternion(
      @w * o.w - @x * o.x - @y * o.y - @z * o.z,
      @w * o.x + @x * o.w + @y * o.z - @z * o.y,
      @w * o.y - @x * o.z + @y * o.w + @z * o.x,
      @w * o.z + @x * o.y - @y * o.x + @z * o.w,
    )
  conj: () -> new Quaternion(@w, -@x, -@y, -@z)
  transform: (v) ->
    q = @multiply(new Quaternion(0, v.x, v.y, v.z))
    q = q.multiply(@conj())
    new Vector3(q.x, q.y, q.z)

quatFromEuler = (alpha, beta, gamma) ->
  x = beta; y = gamma; z = alpha
  cX = Math.cos x / 2
  cY = Math.cos y / 2
  cZ = Math.cos z / 2
  sX = Math.sin x / 2
  sY = Math.sin y / 2
  sZ = Math.sin z / 2
  w = cX * cY * cZ - sX * sY * sZ
  x = sX * cY * cZ - cX * sY * sZ
  y = cX * sY * cZ + sX * cY * sZ
  z = cX * cY * sZ + sX * sY * cZ
  new Quaternion(w, x, y, z)

class MHClient
  constructor: (@session = null, @endpoint = null) ->
    @onerror = null
    @onclose = null
    @onstatechange = null
    @onstartup = null
    @state = 'connecting'
    @lastPingNumber = 0
    @lastPongNumber = 0
    @onping = null

    unless @endpoint?
      @endpoint = "ws://#{window.location.host}/mhctl"

    @socket = new WebSocket(@endpoint, 'mhctl')
    @socket.onmessage = (e) => @_onMessage(e.data)
    @socket.onclose = () => 
      @state = 'disconnected'
      @onstatechange?(@state)
      @onclose?()
    @socket.onerror = () => @onerror?("Network error.")

    @socket.onopen = () =>
      @state = 'connected'
      @onstatechange?(@state)
      if @session?
        @send
          type: 'session'
          session: @session
      return

    return

  close: () -> 
    @socket.close()

  send: (obj) ->
    data = JSON.stringify(obj)
    @socket.send data
    return

  ping: ->
    if @lastPongNumber isnt @lastPingNumber
      @onping?(null, @socket.bufferedAmount)
    @lastPingNumber++
    @lastPingTime = new Date().getTime()
    @send
      type: 'ping'
      data: @lastPingNumber

  _gotPacket: (data) ->
    switch data.type
      when 'accepted'
        @session = data.session
        @state = 'accepted'
        @onstatechange?(@state)
      when 'start'
        @onstartup?(data.code, w: data.screen_width, h: data.screen_height)
      when 'kicked'
        @session = null
        @state = 'disconnected'
        @onstatechange?(@state)
        @onerror?(data.reason)
        @close()
      when 'pong' 
        # ping response
        if data.data is @lastPingNumber
          @lastPongNumber = @lastPingNumber
          @onping?(new Date().getTime() - @lastPingTime, 
          @socket.bufferedAmount)
      else
        try console.warn("unknown cmd: #{data.type}")
    return

  _onMessage: (data) ->
    obj = JSON.parse data
    @_gotPacket obj if obj?
    return

class GyroDevice
  constructor: ->
    @raw = new Quaternion(1, 0, 0, 0)
    @zero = new Quaternion(1, 0, 0, 0)
    deviceOrientationHandler = (e) =>

      s = Math.PI / 180
      @raw = quatFromEuler e.alpha*s, e.beta*s, e.gamma*s

      return
    window.addEventListener 'deviceorientation', deviceOrientationHandler, true
    return
  calibrate: ->
    @zero = @raw
    return
  get: ->
    @zero.conj().multiply(@raw)

$ ->
  client = null

  gyro = new GyroDevice
  minPing = null
  badCount = 0
  lastPing = null
  pingLevel = 0
  timeCount = 0
  screenSize = {w: 1024, h: 768}

  createClient = ->
    if client?
      client = new MHClient(client.session, client.endpoint)
    else
      client = new MHClient()
    minPing = null
    cli = client
    badCount = 0
    pingLevel = 0
    timeCount = 0
    client.onstatechange = (state) ->
      return unless cli is client

      if state is 'connected'
        $('#codeView').removeClass 'accepted'
        $('#codeView').removeClass 'error'
        $('#codeView').text '--------'

      if state is 'accepted'
        timeCount = 0
        $('#codeView').addClass 'accepted'
        $('#codeView').removeClass 'error'
        $('#codeView').text '        '

      if state is 'disconnected'
        # retry
        $('#codeView').removeClass 'accepted'
        $('#codeView').addClass 'error'
        $('#codeView').text '--------'
        client = null
        setTimeout (->
          createClient()
        ), 2000

      $('#stateView').text(state)
      return
    client.onstartup = (code, screenSiz) ->
      return unless cli is client
      $('#codeView').removeClass 'error'
      screenSize = screenSiz
      if client.state is 'connected'
        $('#codeView').text(code)
      return
    client.onping = (rtt, buffer) ->
      $('#pingView').text(rtt + " " + buffer)
      lastPing = rtt
      minPing = Math.max(rtt, 20) if not minPing? or (rtt? and rtt < minPing)
      if not rtt? or rtt > minPing * 5 and
      client.state is 'accepted'
        badCount++
        if badCount >= 6
          # connection getting worse; try reconnection
          client.close()
          setTimeout createClient, 50
      else
        badCount = 0
      return
    return

  lastDx = null; lastDy = null
  sendState = () ->
    return unless client?
    # update gyro state

    nodata = () ->
      # interrupting transmission seems raises ping
      client.send
        type: 'keep_alive'
        data: (Math.random() * 256) | 0

      lastDx = null
      lastDy = null
      return

    if client.state is 'accepted'
      s = gyro.get()
      v = s.transform(new Vector3(0, 1, 0))
      if v.y > 0
        # device is looking at screen side
        sx = v.x / v.y
        sy = v.z / v.y

        range = Math.tan 20 * Math.PI / 180
        sx /= range
        sy /= range

        sw = screenSize.w; sh = screenSize.h; ssize = Math.max sw, sh
        dx = sx * (ssize / 2) + sw / 2
        dy = -sy * (ssize / 2) + sh / 2

        dx |= 0; dy |= 0

        dx = 0 if dx < 0 and dx > -100
        dx = sw if dx >= sw and dx < sw + 100
        dy = 0 if dy < 0 and dy > -100
        dy = sh if dy >= sh and dy < sh + 100

        if 0 <= dx < sw and 0 <= dy < sh
          lastDx = dx unless lastDx?
          lastDy = dy unless lastDy?
          lastDx += (dx - lastDx) * 0.5
          lastDy += (dy - lastDy) * 0.5
          client.send
            type: 'mouse_motion'
            x: lastDx|0
            y: lastDy|0
        else
          nodata()
      else
        nodata()
    else
      lastDx = null; lastDy = null

    return

  sendStateTimer = setInterval sendState, 50

  doPing = () ->
    return unless client?
    if client.state in ['connected', 'accepted']
      client.ping()
    return

  setInterval doPing, 500 

  updateStatus = () ->
    return unless client?
    return if client.state isnt 'accepted'
    text = '        '
    instable = false
    if timeCount < 4
      text = ['        ', '   __   ', '  ____  ', ' ______ '][timeCount];
    else
      if lastPing?
        newPingLevel = 8 - (lastPing - 30) / 300 * 8
        newPingLevel = Math.max Math.min(newPingLevel, 8), 1
      else
        newPingLevel = 0
      pingLevel++ if newPingLevel > pingLevel
      pingLevel-- if newPingLevel < pingLevel
      text = '0'+((if i<=pingLevel then '0' else '_') for i in [2..8]).join ''
      instable = pingLevel is 0
    $('#codeView').text text
    $('#codeView').toggleClass 'instable', instable
    timeCount += 1
    return

  setInterval updateStatus, 100

  $('#calibrate').click -> 
    $('#calibrate').stop().css
      'background-color': '#2080f0',
      'color': '#f0f0f0'
    .animate
      'background-color': '#eaeaea',
      'color': '#404040'
    , 1000

    gyro.calibrate()

  $('#mainView').bind
    touchstart: (e) ->
      e.preventDefault()
      if client? and client.state is 'accepted'
        client.send
          type: 'mouse_button'
          button: 'left'
          down: true
      return
    touchmove: (e) ->
      e.preventDefault()
      return
    touchend: (e) ->
      e.preventDefault()
      if client? and client.state is 'accepted'
        client.send
          type: 'mouse_button'
          button: 'left'
          down: false
      return

  createClient()

  return
