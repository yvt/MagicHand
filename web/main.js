// Generated by CoffeeScript 1.7.1
(function() {
  var GyroDevice, MHClient, Quaternion, Vector3, quatFromEuler;

  Vector3 = (function() {
    function Vector3(x, y, z) {
      this.x = x;
      this.y = y;
      this.z = z;
      return;
    }

    return Vector3;

  })();

  Quaternion = (function() {
    function Quaternion(w, x, y, z) {
      this.w = w;
      this.x = x;
      this.y = y;
      this.z = z;
      return;
    }

    Quaternion.prototype.multiply = function(o) {
      return new Quaternion(this.w * o.w - this.x * o.x - this.y * o.y - this.z * o.z, this.w * o.x + this.x * o.w + this.y * o.z - this.z * o.y, this.w * o.y - this.x * o.z + this.y * o.w + this.z * o.x, this.w * o.z + this.x * o.y - this.y * o.x + this.z * o.w);
    };

    Quaternion.prototype.conj = function() {
      return new Quaternion(this.w, -this.x, -this.y, -this.z);
    };

    Quaternion.prototype.transform = function(v) {
      var q;
      q = this.multiply(new Quaternion(0, v.x, v.y, v.z));
      q = q.multiply(this.conj());
      return new Vector3(q.x, q.y, q.z);
    };

    return Quaternion;

  })();

  quatFromEuler = function(alpha, beta, gamma) {
    var cX, cY, cZ, sX, sY, sZ, w, x, y, z;
    x = beta;
    y = gamma;
    z = alpha;
    cX = Math.cos(x / 2);
    cY = Math.cos(y / 2);
    cZ = Math.cos(z / 2);
    sX = Math.sin(x / 2);
    sY = Math.sin(y / 2);
    sZ = Math.sin(z / 2);
    w = cX * cY * cZ - sX * sY * sZ;
    x = sX * cY * cZ - cX * sY * sZ;
    y = cX * sY * cZ + sX * cY * sZ;
    z = cX * cY * sZ + sX * sY * cZ;
    return new Quaternion(w, x, y, z);
  };

  MHClient = (function() {
    function MHClient(session, endpoint) {
      this.session = session != null ? session : null;
      this.endpoint = endpoint != null ? endpoint : null;
      this.onerror = null;
      this.onclose = null;
      this.onstatechange = null;
      this.onstartup = null;
      this.state = 'connecting';
      this.lastPingNumber = 0;
      this.lastPongNumber = 0;
      this.onping = null;
      if (this.endpoint == null) {
        this.endpoint = "ws://" + window.location.host + "/mhctl";
      }
      this.socket = new WebSocket(this.endpoint, 'mhctl');
      this.socket.onmessage = (function(_this) {
        return function(e) {
          return _this._onMessage(e.data);
        };
      })(this);
      this.socket.onclose = (function(_this) {
        return function() {
          _this.state = 'disconnected';
          if (typeof _this.onstatechange === "function") {
            _this.onstatechange(_this.state);
          }
          return typeof _this.onclose === "function" ? _this.onclose() : void 0;
        };
      })(this);
      this.socket.onerror = (function(_this) {
        return function() {
          return typeof _this.onerror === "function" ? _this.onerror("Network error.") : void 0;
        };
      })(this);
      this.socket.onopen = (function(_this) {
        return function() {
          _this.state = 'connected';
          if (typeof _this.onstatechange === "function") {
            _this.onstatechange(_this.state);
          }
          if (_this.session != null) {
            _this.send({
              type: 'session',
              session: _this.session
            });
          }
        };
      })(this);
      return;
    }

    MHClient.prototype.close = function() {
      return this.socket.close();
    };

    MHClient.prototype.send = function(obj) {
      var data;
      data = JSON.stringify(obj);
      this.socket.send(data);
    };

    MHClient.prototype.ping = function() {
      if (this.lastPongNumber !== this.lastPingNumber) {
        if (typeof this.onping === "function") {
          this.onping(null, this.socket.bufferedAmount);
        }
      }
      this.lastPingNumber++;
      this.lastPingTime = new Date().getTime();
      return this.send({
        type: 'ping',
        data: this.lastPingNumber
      });
    };

    MHClient.prototype._gotPacket = function(data) {
      switch (data.type) {
        case 'accepted':
          this.session = data.session;
          this.state = 'accepted';
          if (typeof this.onstatechange === "function") {
            this.onstatechange(this.state);
          }
          break;
        case 'start':
          if (typeof this.onstartup === "function") {
            this.onstartup(data.code, {
              w: data.screen_width,
              h: data.screen_height
            });
          }
          break;
        case 'kicked':
          this.session = null;
          this.state = 'disconnected';
          if (typeof this.onstatechange === "function") {
            this.onstatechange(this.state);
          }
          if (typeof this.onerror === "function") {
            this.onerror(data.reason);
          }
          this.close();
          break;
        case 'pong':
          if (data.data === this.lastPingNumber) {
            this.lastPongNumber = this.lastPingNumber;
            if (typeof this.onping === "function") {
              this.onping(new Date().getTime() - this.lastPingTime, this.socket.bufferedAmount);
            }
          }
          break;
        default:
          try {
            console.warn("unknown cmd: " + data.type);
          } catch (_error) {}
      }
    };

    MHClient.prototype._onMessage = function(data) {
      var obj;
      obj = JSON.parse(data);
      if (obj != null) {
        this._gotPacket(obj);
      }
    };

    return MHClient;

  })();

  GyroDevice = (function() {
    function GyroDevice() {
      var deviceOrientationHandler;
      this.raw = new Quaternion(1, 0, 0, 0);
      this.zero = new Quaternion(1, 0, 0, 0);
      deviceOrientationHandler = (function(_this) {
        return function(e) {
          var s;
          s = Math.PI / 180;
          _this.raw = quatFromEuler(e.alpha * s, e.beta * s, e.gamma * s);
        };
      })(this);
      window.addEventListener('deviceorientation', deviceOrientationHandler, true);
      return;
    }

    GyroDevice.prototype.calibrate = function() {
      this.zero = this.raw;
    };

    GyroDevice.prototype.get = function() {
      return this.zero.conj().multiply(this.raw);
    };

    return GyroDevice;

  })();

  $(function() {
    var badCount, client, createClient, doPing, gyro, lastDx, lastDy, lastPing, minPing, pingLevel, screenSize, sendState, sendStateTimer, timeCount, updateStatus;
    client = null;
    gyro = new GyroDevice;
    minPing = null;
    badCount = 0;
    lastPing = null;
    pingLevel = 0;
    timeCount = 0;
    screenSize = {
      w: 1024,
      h: 768
    };
    createClient = function() {
      var cli;
      if (client != null) {
        client = new MHClient(client.session, client.endpoint);
      } else {
        client = new MHClient();
      }
      minPing = null;
      cli = client;
      badCount = 0;
      pingLevel = 0;
      timeCount = 0;
      client.onstatechange = function(state) {
        if (cli !== client) {
          return;
        }
        if (state === 'connected') {
          $('#codeView').removeClass('accepted');
          $('#codeView').removeClass('error');
          $('#codeView').text('--------');
        }
        if (state === 'accepted') {
          timeCount = 0;
          $('#codeView').addClass('accepted');
          $('#codeView').removeClass('error');
          $('#codeView').text('        ');
        }
        if (state === 'disconnected') {
          $('#codeView').removeClass('accepted');
          $('#codeView').addClass('error');
          $('#codeView').text('--------');
          client = null;
          setTimeout((function() {
            return createClient();
          }), 2000);
        }
        $('#stateView').text(state);
      };
      client.onstartup = function(code, screenSiz) {
        if (cli !== client) {
          return;
        }
        $('#codeView').removeClass('error');
        screenSize = screenSiz;
        if (client.state === 'connected') {
          $('#codeView').text(code);
        }
      };
      client.onping = function(rtt, buffer) {
        $('#pingView').text(rtt + " " + buffer);
        lastPing = rtt;
        if ((minPing == null) || ((rtt != null) && rtt < minPing)) {
          minPing = Math.max(rtt, 20);
        }
        if ((rtt == null) || rtt > minPing * 5 && client.state === 'accepted') {
          badCount++;
          if (badCount >= 6) {
            client.close();
            setTimeout(createClient, 50);
          }
        } else {
          badCount = 0;
        }
      };
    };
    lastDx = null;
    lastDy = null;
    sendState = function() {
      var dx, dy, nodata, range, s, sh, ssize, sw, sx, sy, v;
      if (client == null) {
        return;
      }
      nodata = function() {
        client.send({
          type: 'keep_alive',
          data: (Math.random() * 256) | 0
        });
        lastDx = null;
        lastDy = null;
      };
      if (client.state === 'accepted') {
        s = gyro.get();
        v = s.transform(new Vector3(0, 1, 0));
        if (v.y > 0) {
          sx = v.x / v.y;
          sy = v.z / v.y;
          range = Math.tan(20 * Math.PI / 180);
          sx /= range;
          sy /= range;
          sw = screenSize.w;
          sh = screenSize.h;
          ssize = Math.max(sw, sh);
          dx = sx * (ssize / 2) + sw / 2;
          dy = -sy * (ssize / 2) + sh / 2;
          dx |= 0;
          dy |= 0;
          if (dx < 0 && dx > -100) {
            dx = 0;
          }
          if (dx >= sw && dx < sw + 100) {
            dx = sw;
          }
          if (dy < 0 && dy > -100) {
            dy = 0;
          }
          if (dy >= sh && dy < sh + 100) {
            dy = sh;
          }
          if ((0 <= dx && dx < sw) && (0 <= dy && dy < sh)) {
            if (lastDx == null) {
              lastDx = dx;
            }
            if (lastDy == null) {
              lastDy = dy;
            }
            lastDx += (dx - lastDx) * 0.5;
            lastDy += (dy - lastDy) * 0.5;
            client.send({
              type: 'mouse_motion',
              x: lastDx | 0,
              y: lastDy | 0
            });
          } else {
            nodata();
          }
        } else {
          nodata();
        }
      } else {
        lastDx = null;
        lastDy = null;
      }
    };
    sendStateTimer = setInterval(sendState, 50);
    doPing = function() {
      var _ref;
      if (client == null) {
        return;
      }
      if ((_ref = client.state) === 'connected' || _ref === 'accepted') {
        client.ping();
      }
    };
    setInterval(doPing, 500);
    updateStatus = function() {
      var i, instable, newPingLevel, text;
      if (client == null) {
        return;
      }
      if (client.state !== 'accepted') {
        return;
      }
      text = '        ';
      instable = false;
      if (timeCount < 4) {
        text = ['        ', '   __   ', '  ____  ', ' ______ '][timeCount];
      } else {
        if (lastPing != null) {
          newPingLevel = 8 - (lastPing - 30) / 300 * 8;
          newPingLevel = Math.max(Math.min(newPingLevel, 8), 1);
        } else {
          newPingLevel = 0;
        }
        if (newPingLevel > pingLevel) {
          pingLevel++;
        }
        if (newPingLevel < pingLevel) {
          pingLevel--;
        }
        text = '0' + ((function() {
          var _i, _results;
          _results = [];
          for (i = _i = 2; _i <= 8; i = ++_i) {
            _results.push(i <= pingLevel ? '0' : '_');
          }
          return _results;
        })()).join('');
        instable = pingLevel === 0;
      }
      $('#codeView').text(text);
      $('#codeView').toggleClass('instable', instable);
      timeCount += 1;
    };
    setInterval(updateStatus, 100);
    $('#calibrate').click(function() {
      $('#calibrate').stop().css({
        'background-color': '#2080f0',
        'color': '#f0f0f0'
      }).animate({
        'background-color': '#eaeaea',
        'color': '#404040'
      }, 1000);
      return gyro.calibrate();
    });
    $('#mainView').bind({
      touchstart: function(e) {
        e.preventDefault();
        if ((client != null) && client.state === 'accepted') {
          client.send({
            type: 'mouse_button',
            button: 'left',
            down: true
          });
        }
      },
      touchmove: function(e) {
        e.preventDefault();
      },
      touchend: function(e) {
        e.preventDefault();
        if ((client != null) && client.state === 'accepted') {
          client.send({
            type: 'mouse_button',
            button: 'left',
            down: false
          });
        }
      }
    });
    createClient();
  });

}).call(this);