// TODO(crbug.com/372866269): Fix ESlint violations and remove exception below.
/* eslint-disable eqeqeq */
const lottiejs = (function(window) {
  'use strict';
  const svgNS = 'http://www.w3.org/2000/svg';

  const locationHref = '';

  const initialDefaultFrame = -999999;

  const subframeEnabled = true;
  let expressionsPlugin = undefined;
  const isSafari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);
  const cachedColors = {};
  const bm_rounder = Math.round;
  let bm_rnd;
  const bm_pow = Math.pow;
  const bm_sqrt = Math.sqrt;
  const bm_abs = Math.abs;
  const bm_floor = Math.floor;
  const bm_max = Math.max;
  const bm_min = Math.min;
  const blitter = 10;

  const BMMath = {};
  (function() {
    const propertyNames = [
      'abs',   'acos',  'acosh',  'asin',   'asinh', 'atan',   'atanh',
      'atan2', 'ceil',  'cbrt',   'expm1',  'clz32', 'cos',    'cosh',
      'exp',   'floor', 'fround', 'hypot',  'imul',  'log',    'log1p',
      'log2',  'log10', 'max',    'min',    'pow',   'random', 'round',
      'sign',  'sin',   'sinh',   'sqrt',   'tan',   'tanh',   'trunc',
      'E',     'LN10',  'LN2',    'LOG10E', 'LOG2E', 'PI',     'SQRT1_2',
      'SQRT2',
    ];
    let i;
    const len = propertyNames.length;
    for(i=0;i<len;i+=1){
      BMMath[propertyNames[i]] = Math[propertyNames[i]];
    }
  }());

  function ProjectInterface() {
    return {};
  }

  BMMath.random = Math.random;
  BMMath.abs = function(val) {
    const tOfVal = typeof val;
    if(tOfVal === 'object' && val.length){
      const absArr = createSizedArray(val.length);
      let i;
      const len = val.length;
      for (i = 0; i < len; i += 1) {
        absArr[i] = Math.abs(val[i]);
      }
      return absArr;
    }
    return Math.abs(val);
  };
  let defaultCurveSegments = 150;
  const degToRads = Math.PI / 180;
  const roundCorner = 0.5519;

  function roundValues(flag) {
    if(flag){
      bm_rnd = Math.round;
    }else{
      bm_rnd = function(val) {
        return val;
      };
    }
  }
  roundValues(false);

  function styleDiv(element) {
    element.style.position = 'absolute';
    element.style.top = 0;
    element.style.left = 0;
    element.style.display = 'block';
    element.style.transformOrigin = element.style.webkitTransformOrigin = '0 0';
    element.style.backfaceVisibility  = element.style.webkitBackfaceVisibility = 'visible';
    element.style.transformStyle = element.style.webkitTransformStyle =
        element.style.mozTransformStyle = 'preserve-3d';
  }

  function BMEnterFrameEvent(n, c, t, d) {
    this.type = n;
    this.currentTime = c;
    this.totalTime = t;
    this.direction = d < 0 ? -1:1;
  }

  function BMCompleteEvent(n, d) {
    this.type = n;
    this.direction = d < 0 ? -1:1;
  }

  function BMCompleteLoopEvent(n, c, t, d) {
    this.type = n;
    this.currentLoop = t;
    this.totalLoops = c;
    this.direction = d < 0 ? -1:1;
  }

  function BMSegmentStartEvent(n, f, t) {
    this.type = n;
    this.firstFrame = f;
    this.totalFrames = t;
  }

  function BMDestroyEvent(n, t) {
    this.type = n;
    this.target = t;
  }

  const createElementID = (function() {
    let _count = 0;
    return function createID() {
      return '__lottie_element_' + ++_count;
    };
  }());

  function HSVtoRGB(h, s, v) {
    let r;
    let g;
    let b;
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    switch (i % 6) {
      case 0:
        r = v;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = v;
        b = p;
        break;
      case 2:
        r = p;
        g = v;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = v;
        break;
      case 4:
        r = t;
        g = p;
        b = v;
        break;
      case 5:
        r = v;
        g = p;
        b = q;
        break;
    }
    return [ r,
        g,
         b ];
  }

  function RGBtoHSV(r, g, b) {
    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    const d = max - min;
    let h;
    const s = (max === 0 ? 0 : d / max);
    const v = max / 255;

    switch (max) {
      case min:
        h = 0;
        break;
      case r:
        h = (g - b) + d * (g < b ? 6 : 0);
        h /= 6 * d;
        break;
      case g:
        h = (b - r) + d * 2;
        h /= 6 * d;
        break;
      case b:
        h = (r - g) + d * 4;
        h /= 6 * d;
        break;
    }

    return [
      h,
      s,
      v,
    ];
  }

  function addSaturationToRGB(color, offset) {
    const hsv = RGBtoHSV(color[0] * 255, color[1] * 255, color[2] * 255);
    hsv[1] += offset;
    if (hsv[1] > 1) {
      hsv[1] = 1;
    } else if (hsv[1] <= 0) {
      hsv[1] = 0;
    }
    return HSVtoRGB(hsv[0],hsv[1],hsv[2]);
  }

  function addBrightnessToRGB(color, offset) {
    const hsv = RGBtoHSV(color[0] * 255, color[1] * 255, color[2] * 255);
    hsv[2] += offset;
    if (hsv[2] > 1) {
      hsv[2] = 1;
    } else if (hsv[2] < 0) {
      hsv[2] = 0;
    }
    return HSVtoRGB(hsv[0],hsv[1],hsv[2]);
  }

  function addHueToRGB(color, offset) {
    const hsv = RGBtoHSV(color[0] * 255, color[1] * 255, color[2] * 255);
    hsv[0] += offset/360;
    if (hsv[0] > 1) {
      hsv[0] -= 1;
    } else if (hsv[0] < 0) {
      hsv[0] += 1;
    }
    return HSVtoRGB(hsv[0],hsv[1],hsv[2]);
  }

  const rgbToHex = (function() {
    const colorMap = [];
    let i;
    let hex;
    for(i=0;i<256;i+=1){
      hex = i.toString(16);
      colorMap[i] = hex.length == 1 ? '0' + hex : hex;
    }

    return function(r, g, b) {
        if(r<0){
            r = 0;
        }
        if(g<0){
            g = 0;
        }
        if(b<0){
            b = 0;
        }
        return '#' + colorMap[r] + colorMap[g] + colorMap[b];
    };
  }());
  function BaseEvent() {}
  BaseEvent.prototype = {
    triggerEvent: function(eventName, args) {
      if (this._cbs[eventName]) {
        const len = this._cbs[eventName].length;
        for (let i = 0; i < len; i++) {
          this._cbs[eventName][i](args);
        }
      }
    },
    addEventListener: function(eventName, callback) {
      if (!this._cbs[eventName]){
        this._cbs[eventName] = [];
      }
      this._cbs[eventName].push(callback);

      return function() {
        this.removeEventListener(eventName, callback);
      }.bind(this);
    },
    removeEventListener: function(eventName, callback) {
      if (!callback){
        this._cbs[eventName] = null;
      }else if(this._cbs[eventName]){
        let i = 0;
        let len = this._cbs[eventName].length;
        while (i < len) {
          if (this._cbs[eventName][i] === callback) {
            this._cbs[eventName].splice(i, 1);
            i -= 1;
            len -= 1;
          }
          i += 1;
        }
        if (!this._cbs[eventName].length) {
          this._cbs[eventName] = null;
        }
      }
    },
  };
  const createTypedArray = (function() {
    function createRegularArray(type, len) {
      let i = 0;
      const arr = [];
      let value;
      switch (type) {
        case 'int16':
        case 'uint8c':
          value = 1;
          break;
        default:
          value = 1.1;
          break;
      }
      for (i = 0; i < len; i += 1) {
        arr.push(value);
      }
      return arr;
    }
    function createTypedArray(type, len) {
      if (type === 'float32') {
        return new Float32Array(len);
      } else if (type === 'int16') {
        return new Int16Array(len);
      } else if (type === 'uint8c') {
        return new Uint8ClampedArray(len);
      }
    }
    if (typeof Uint8ClampedArray === 'function' &&
        typeof Float32Array === 'function') {
      return createTypedArray;
    } else {
      return createRegularArray;
    }
  }());

  function createSizedArray(len) {
    return Array.apply(null, {length: len});
  }
  function createTag(type) {
    // return {appendChild:function(){},setAttribute:function(){},style:{}}
    return document.createElement(type);
  }
  function DynamicPropertyContainer() {}
  DynamicPropertyContainer.prototype = {
    addDynamicProperty: function(prop) {
      if (this.dynamicProperties.indexOf(prop) === -1) {
        this.dynamicProperties.push(prop);
        this.container.addDynamicProperty(this);
        this._isAnimated = true;
      }
    },
    iterateDynamicProperties: function() {
      this._mdf = false;
      let i;
      const len = this.dynamicProperties.length;
      for(i=0;i<len;i+=1){
        this.dynamicProperties[i].getValue();
        if (this.dynamicProperties[i]._mdf) {
          this._mdf = true;
        }
      }
    },
    initDynamicPropertyContainer: function(container) {
      this.container = container;
      this.dynamicProperties = [];
      this._mdf = false;
      this._isAnimated = false;
    },
  };
  const getBlendMode = (function() {
    const blendModeEnums = {
      0: 'source-over',
      1: 'multiply',
      2: 'screen',
      3: 'overlay',
      4: 'darken',
      5: 'lighten',
      6: 'color-dodge',
      7: 'color-burn',
      8: 'hard-light',
      9: 'soft-light',
      10: 'difference',
      11: 'exclusion',
      12: 'hue',
      13: 'saturation',
      14: 'color',
      15: 'luminosity',
    };

    return function(mode) {
      return blendModeEnums[mode] || '';
    };
  }());

  /**
   * 2D transformation matrix object initialized with identity matrix.
   *
   * The matrix can synchronize a canvas context by supplying the context
   * as an argument, or later apply current absolute transform to an
   * existing context.
   *
   * All values are handled as floating point values.
   *
   * @param {CanvasRenderingContext2D} [context] - Optional context to sync with
   *     Matrix
   * @prop {number} a - scale x
   * @prop {number} b - shear y
   * @prop {number} c - shear x
   * @prop {number} d - scale y
   * @prop {number} e - translate x
   * @prop {number} f - translate y
   * @prop {CanvasRenderingContext2D|null} [context=null] - set or get current
   * canvas context
   * @constructor
   */

  const Matrix =
      (function() {
        const _cos = Math.cos;
        const _sin = Math.sin;
        const _tan = Math.tan;
        const _rnd = Math.round;

        function reset() {
          this.props[0] = 1;
          this.props[1] = 0;
          this.props[2] = 0;
          this.props[3] = 0;
          this.props[4] = 0;
          this.props[5] = 1;
          this.props[6] = 0;
          this.props[7] = 0;
          this.props[8] = 0;
          this.props[9] = 0;
          this.props[10] = 1;
          this.props[11] = 0;
          this.props[12] = 0;
          this.props[13] = 0;
          this.props[14] = 0;
          this.props[15] = 1;
          return this;
        }

        function rotate(angle) {
          if (angle === 0) {
            return this;
          }
          const mCos = _cos(angle);
          const mSin = _sin(angle);
          return this._t(
              mCos, -mSin, 0, 0, mSin, mCos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        }

        function rotateX(angle) {
          if (angle === 0) {
            return this;
          }
          const mCos = _cos(angle);
          const mSin = _sin(angle);
          return this._t(
              1, 0, 0, 0, 0, mCos, -mSin, 0, 0, mSin, mCos, 0, 0, 0, 0, 1);
        }

        function rotateY(angle) {
          if (angle === 0) {
            return this;
          }
          const mCos = _cos(angle);
          const mSin = _sin(angle);
          return this._t(
              mCos, 0, mSin, 0, 0, 1, 0, 0, -mSin, 0, mCos, 0, 0, 0, 0, 1);
        }

        function rotateZ(angle) {
          if (angle === 0) {
            return this;
          }
          const mCos = _cos(angle);
          const mSin = _sin(angle);
          return this._t(
              mCos, -mSin, 0, 0, mSin, mCos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        }

        function shear(sx, sy) {
          return this._t(1, sy, sx, 1, 0, 0);
        }

        function skew(ax, ay) {
          return this.shear(_tan(ax), _tan(ay));
        }

        function skewFromAxis(ax, angle) {
          const mCos = _cos(angle);
          const mSin = _sin(angle);
          return this
              ._t(mCos, mSin, 0, 0, -mSin, mCos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
              ._t(1, 0, 0, 0, _tan(ax), 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
              ._t(mCos, -mSin, 0, 0, mSin, mCos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
          // return this._t(mCos, mSin, -mSin, mCos, 0, 0)._t(1, 0, _tan(ax), 1,
          // 0, 0)._t(mCos, -mSin, mSin, mCos, 0, 0);
        }

        function scale(sx, sy, sz) {
          if (!sz && sz !== 0) {
            sz = 1;
          }
          if (sx === 1 && sy === 1 && sz === 1) {
            return this;
          }
          return this._t(sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1);
        }

        function setTransform(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {
          this.props[0] = a;
          this.props[1] = b;
          this.props[2] = c;
          this.props[3] = d;
          this.props[4] = e;
          this.props[5] = f;
          this.props[6] = g;
          this.props[7] = h;
          this.props[8] = i;
          this.props[9] = j;
          this.props[10] = k;
          this.props[11] = l;
          this.props[12] = m;
          this.props[13] = n;
          this.props[14] = o;
          this.props[15] = p;
          return this;
        }

        function translate(tx, ty, tz) {
          tz = tz || 0;
          if (tx !== 0 || ty !== 0 || tz !== 0) {
            return this._t(1,0,0,0,0,1,0,0,0,0,1,0,tx,ty,tz,1);
          }
          return this;
        }

        function transform(
            a2, b2, c2, d2, e2, f2, g2, h2, i2, j2, k2, l2, m2, n2, o2, p2) {
          const _p = this.props;

          if (a2 === 1 && b2 === 0 && c2 === 0 && d2 === 0 && e2 === 0 &&
              f2 === 1 && g2 === 0 && h2 === 0 && i2 === 0 && j2 === 0 &&
              k2 === 1 && l2 === 0) {
            //NOTE: commenting this condition because TurboFan deoptimizes code when present
            //if(m2 !== 0 || n2 !== 0 || o2 !== 0){
            _p[12] = _p[12] * a2 + _p[15] * m2;
            _p[13] = _p[13] * f2 + _p[15] * n2;
            _p[14] = _p[14] * k2 + _p[15] * o2;
            _p[15] = _p[15] * p2;
            //}
            this._identityCalculated = false;
            return this;
          }

          const a1 = _p[0];
          const b1 = _p[1];
          const c1 = _p[2];
          const d1 = _p[3];
          const e1 = _p[4];
          const f1 = _p[5];
          const g1 = _p[6];
          const h1 = _p[7];
          const i1 = _p[8];
          const j1 = _p[9];
          const k1 = _p[10];
          const l1 = _p[11];
          const m1 = _p[12];
          const n1 = _p[13];
          const o1 = _p[14];
          const p1 = _p[15];

          /* matrix order (canvas compatible):
           * ace
           * bdf
           * 001
           */
          _p[0] = a1 * a2 + b1 * e2 + c1 * i2 + d1 * m2;
          _p[1] = a1 * b2 + b1 * f2 + c1 * j2 + d1 * n2;
          _p[2] = a1 * c2 + b1 * g2 + c1 * k2 + d1 * o2;
          _p[3] = a1 * d2 + b1 * h2 + c1 * l2 + d1 * p2;

          _p[4] = e1 * a2 + f1 * e2 + g1 * i2 + h1 * m2;
          _p[5] = e1 * b2 + f1 * f2 + g1 * j2 + h1 * n2;
          _p[6] = e1 * c2 + f1 * g2 + g1 * k2 + h1 * o2;
          _p[7] = e1 * d2 + f1 * h2 + g1 * l2 + h1 * p2;

          _p[8] = i1 * a2 + j1 * e2 + k1 * i2 + l1 * m2;
          _p[9] = i1 * b2 + j1 * f2 + k1 * j2 + l1 * n2;
          _p[10] = i1 * c2 + j1 * g2 + k1 * k2 + l1 * o2;
          _p[11] = i1 * d2 + j1 * h2 + k1 * l2 + l1 * p2;

          _p[12] = m1 * a2 + n1 * e2 + o1 * i2 + p1 * m2;
          _p[13] = m1 * b2 + n1 * f2 + o1 * j2 + p1 * n2;
          _p[14] = m1 * c2 + n1 * g2 + o1 * k2 + p1 * o2;
          _p[15] = m1 * d2 + n1 * h2 + o1 * l2 + p1 * p2;

          this._identityCalculated = false;
          return this;
        }

        function isIdentity() {
          if (!this._identityCalculated) {
            this._identity = !(this.props[0] !== 1 || this.props[1] !== 0 || this.props[2] !== 0 || this.props[3] !== 0 || this.props[4] !== 0 || this.props[5] !== 1 || this.props[6] !== 0 || this.props[7] !== 0 || this.props[8] !== 0 || this.props[9] !== 0 || this.props[10] !== 1 || this.props[11] !== 0 || this.props[12] !== 0 || this.props[13] !== 0 || this.props[14] !== 0 || this.props[15] !== 1);
            this._identityCalculated = true;
          }
          return this._identity;
        }

        function equals(matr) {
          let i = 0;
          while (i < 16) {
            if(matr.props[i] !== this.props[i]) {
              return false;
            }
            i+=1;
          }
          return true;
        }

        function clone(matr) {
          let i;
          for (i = 0; i < 16; i += 1) {
            matr.props[i] = this.props[i];
          }
        }

        function cloneFromProps(props) {
          let i;
          for (i = 0; i < 16; i += 1) {
            this.props[i] = props[i];
          }
        }

        function applyToPoint(x, y, z) {
          return {
            x: x * this.props[0] + y * this.props[4] + z * this.props[8] +
                this.props[12],
            y: x * this.props[1] + y * this.props[5] + z * this.props[9] +
                this.props[13],
            z: x * this.props[2] + y * this.props[6] + z * this.props[10] +
                this.props[14],
          };
          /*return {
           x: x * me.a + y * me.c + me.e,
           y: x * me.b + y * me.d + me.f
           };*/
        }
        function applyToX(x, y, z) {
          return x * this.props[0] + y * this.props[4] + z * this.props[8] +
              this.props[12];
        }
        function applyToY(x, y, z) {
          return x * this.props[1] + y * this.props[5] + z * this.props[9] +
              this.props[13];
        }
        function applyToZ(x, y, z) {
          return x * this.props[2] + y * this.props[6] + z * this.props[10] +
              this.props[14];
        }

        function inversePoint(pt) {
          const determinant =
              this.props[0] * this.props[5] - this.props[1] * this.props[4];
          const a = this.props[5] / determinant;
          const b = -this.props[1] / determinant;
          const c = -this.props[4] / determinant;
          const d = this.props[0] / determinant;
          const e = (this.props[4] * this.props[13] -
                     this.props[5] * this.props[12]) /
              determinant;
          const f = -(this.props[0] * this.props[13] -
                      this.props[1] * this.props[12]) /
              determinant;
          return [pt[0] * a + pt[1] * c + e, pt[0] * b + pt[1] * d + f, 0];
        }

        function inversePoints(pts) {
          let i;
          const len = pts.length;
          const retPts = [];
          for (i = 0; i < len; i += 1) {
            retPts[i] = inversePoint(pts[i]);
          }
          return retPts;
        }

        function applyToTriplePoints(pt1, pt2, pt3) {
          const arr = createTypedArray('float32', 6);
          if (this.isIdentity()) {
            arr[0] = pt1[0];
            arr[1] = pt1[1];
            arr[2] = pt2[0];
            arr[3] = pt2[1];
            arr[4] = pt3[0];
            arr[5] = pt3[1];
          } else {
            const p0 = this.props[0];
            const p1 = this.props[1];
            const p4 = this.props[4];
            const p5 = this.props[5];
            const p12 = this.props[12];
            const p13 = this.props[13];
            arr[0] = pt1[0] * p0 + pt1[1] * p4 + p12;
            arr[1] = pt1[0] * p1 + pt1[1] * p5 + p13;
            arr[2] = pt2[0] * p0 + pt2[1] * p4 + p12;
            arr[3] = pt2[0] * p1 + pt2[1] * p5 + p13;
            arr[4] = pt3[0] * p0 + pt3[1] * p4 + p12;
            arr[5] = pt3[0] * p1 + pt3[1] * p5 + p13;
          }
          return arr;
        }

        function applyToPointArray(x, y, z) {
          let arr;
          if (this.isIdentity()) {
            arr = [x,y,z];
          } else {
            arr = [x * this.props[0] + y * this.props[4] + z * this.props[8] + this.props[12],x * this.props[1] + y * this.props[5] + z * this.props[9] + this.props[13],x * this.props[2] + y * this.props[6] + z * this.props[10] + this.props[14]];
          }
          return arr;
        }

        function applyToPointStringified(x, y) {
          if (this.isIdentity()) {
            return x + ',' + y;
          }
          const _p = this.props;
          return Math.round((x * _p[0] + y * _p[4] + _p[12]) * 100) / 100 +
              ',' + Math.round((x * _p[1] + y * _p[5] + _p[13]) * 100) / 100;
        }

        function toCSS() {
          // Doesn't make much sense to add this optimization. If it is an
          // identity matrix, it's very likely this will get called only once
          // since it won't be keyframed.
          /*if(this.isIdentity()) {
              return '';
          }*/
          let i = 0;
          const props = this.props;
          let cssValue = 'matrix3d(';
          const v = 10000;
          while (i < 16) {
            cssValue += _rnd(props[i]*v)/v;
            cssValue += i === 15 ? ')':',';
            i += 1;
          }
          return cssValue;
        }

        function roundMatrixProperty(val) {
          const v = 10000;
          if ((val < 0.000001 && val > 0) || (val > -0.000001 && val < 0)) {
            return _rnd(val * v) / v;
          }
          return val;
        }

        function to2dCSS() {
          // Doesn't make much sense to add this optimization. If it is an
          // identity matrix, it's very likely this will get called only once
          // since it won't be keyframed.
          /*if(this.isIdentity()) {
              return '';
          }*/
          const props = this.props;
          const _a = roundMatrixProperty(props[0]);
          const _b = roundMatrixProperty(props[1]);
          const _c = roundMatrixProperty(props[4]);
          const _d = roundMatrixProperty(props[5]);
          const _e = roundMatrixProperty(props[12]);
          const _f = roundMatrixProperty(props[13]);
          return 'matrix(' + _a + ',' + _b + ',' + _c + ',' + _d + ',' + _e +
              ',' + _f + ')';
        }

        return function() {
          this.reset = reset;
          this.rotate = rotate;
          this.rotateX = rotateX;
          this.rotateY = rotateY;
          this.rotateZ = rotateZ;
          this.skew = skew;
          this.skewFromAxis = skewFromAxis;
          this.shear = shear;
          this.scale = scale;
          this.setTransform = setTransform;
          this.translate = translate;
          this.transform = transform;
          this.applyToPoint = applyToPoint;
          this.applyToX = applyToX;
          this.applyToY = applyToY;
          this.applyToZ = applyToZ;
          this.applyToPointArray = applyToPointArray;
          this.applyToTriplePoints = applyToTriplePoints;
          this.applyToPointStringified = applyToPointStringified;
          this.toCSS = toCSS;
          this.to2dCSS = to2dCSS;
          this.clone = clone;
          this.cloneFromProps = cloneFromProps;
          this.equals = equals;
          this.inversePoints = inversePoints;
          this.inversePoint = inversePoint;
          this._t = this.transform;
          this.isIdentity = isIdentity;
          this._identity = true;
          this._identityCalculated = false;

          this.props = createTypedArray('float32', 16);
          this.reset();
        };
      }());

  (function(pool, math) {
    //
    // The following constants are related to IEEE 754 limits.
    //
    const global = this;
    const width = 256;  // each RC4 output is 0 <= x < 256
    const chunks = 6;   // at least six RC4 outputs for each double
    const digits = 52;  // there are 52 significant digits in a double
    const rngname =
        'random';  // rngname: name for Math.random and Math.seedrandom
    const startdenom = math.pow(width, chunks);
    const significance = math.pow(2, digits);
    const overflow = significance * 2;
    const mask = width - 1;
    let nodecrypto;  // node.js crypto module, initialized at the bottom.

    //
    // seedrandom()
    // This is the seedrandom function described above.
    //
    function seedrandom(seed, options, callback) {
      const key = [];
      options = (options === true) ? {entropy: true} : (options || {});

      // Flatten the seed string or build one from local entropy if needed.
      const shortseed = mixkey(
          flatten(
              options.entropy     ? [seed, tostring(pool)] :
                  (seed === null) ? autoseed() :
                                    seed,
              3),
          key);

      // Use the seed to initialize an ARC4 generator.
      const arc4 = new ARC4(key);

      // This function returns a random double in [0, 1) that contains
      // randomness in every bit of the mantissa of the IEEE 754 value.
      const prng = function() {
        let n = arc4.g(chunks);     // Start with a numerator n < 2 ^ 48
        let d = startdenom;         //   and denominator d = 2 ^ 48.
        let x = 0;                  //   and no 'extra last byte'.
        while (n < significance) {  // Fill up all significant digits by
          n = (n + x) * width;      //   shifting numerator and
          d *= width;               //   denominator and generating a
          x = arc4.g(1);            //   new least-significant-byte.
        }
        while (n >= overflow) {  // To avoid rounding up, before adding
          n /= 2;                //   last byte, shift everything
          d /= 2;                //   right using integer math until
          x >>>= 1;              //   we have exactly the desired bits.
        }
        return (n + x) / d;  // Form the number within [0, 1).
      };

      prng.int32 = function() {
        return arc4.g(4) | 0;
      };
      prng.quick = function() {
        return arc4.g(4) / 0x100000000;
      };
      prng.double = prng;

      // Mix the randomness into accumulated entropy.
      mixkey(tostring(arc4.S), pool);

      // Calling convention: what to return as a function of prng, seed,
      // is_math.
      return (
          options.pass || callback ||
          function(prng, seed, is_math_call, state) {
            if (state) {
              // Load the arc4 state from the given state if it has an S array.
              if (state.S) {
                copy(state, arc4);
              }
              // Only provide the .state method if requested via options.state.
              prng.state = function() {
                return copy(arc4, {});
              };
            }

            if (is_math_call) {
              // If called as a method of Math (Math.seedrandom()), mutate
              // Math.random because that is how seedrandom.js has worked since
              // v1.0.
              math[rngname] = prng;
              return seed;
            } else {
              // Otherwise, it is a newer calling convention, so return the prng
              // directly.
              return prng;
            }
          })(
          prng, shortseed,
          'global' in options ? options.global : (this == math), options.state);
    }
    math['seed' + rngname] = seedrandom;

    //
    // ARC4
    //
    // An ARC4 implementation.  The constructor takes a key in the form of
    // an array of at most (width) integers that should be 0 <= x < (width).
    //
    // The g(count) method returns a pseudorandom integer that concatenates
    // the next (count) outputs from ARC4.  Its return value is a number x
    // that is in the range 0 <= x < (width ^ count).
    //
    function ARC4(key) {
      let t;
      let keylen = key.length;
      const me = this;
      let i = 0;
      let j = me.i = me.j = 0;
      const s = me.S = [];

      // The empty key [] is treated as [0].
      if (!keylen) {
        key = [keylen++];
      }

      // Set up S using the standard key scheduling algorithm.
      while (i < width) {
        s[i] = i++;
      }
      for (i = 0; i < width; i++) {
        s[i] = s[j = mask & (j + key[i % keylen] + (t = s[i]))];
        s[j] = t;
      }

      // The "g" method returns the next (count) outputs as one number.
      me.g = function(count) {
        // Using instance members instead of closure state nearly doubles speed.
        let t;
        let r = 0;
        let i = me.i;
        let j = me.j;
        const s = me.S;
        while (count--) {
          t = s[i = mask & (i + 1)];
          r = r * width +
              s[mask & ((s[i] = s[j = mask & (j + t)]) + (s[j] = t))];
        }
        me.i = i;
        me.j = j;
        return r;
        // For robust unpredictability, the function call below automatically
        // discards an initial batch of values.  This is called RC4-drop[256].
        // See http://google.com/search?q=rsa+fluhrer+response&btnI
      };
    }

    //
    // copy()
    // Copies internal state of ARC4 to or from a plain object.
    //
    function copy(f, t) {
      t.i = f.i;
      t.j = f.j;
      t.S = f.S.slice();
      return t;
    }

    //
    // flatten()
    // Converts an object tree to nested arrays of strings.
    //
    function flatten(obj, depth) {
      const result = [];
      const typ = (typeof obj);
      let prop;
      if (depth && typ == 'object') {
        for (prop in obj) {
          try {
            result.push(flatten(obj[prop], depth - 1));
          } catch (e) {
          }
        }
      }
      return (result.length ? result : typ == 'string' ? obj : obj + '\0');
    }

    //
    // mixkey()
    // Mixes a string seed into a key that is an array of integers, and
    // returns a shortened string seed that is equivalent to the result key.
    //
    function mixkey(seed, key) {
      const stringseed = seed + '';
      let smear;
      let j = 0;
      while (j < stringseed.length) {
        key[mask & j] =
            mask & ((smear ^= key[mask & j] * 19) + stringseed.charCodeAt(j++));
      }
      return tostring(key);
    }

    //
    // autoseed()
    // Returns an object for autoseeding, using window.crypto and Node crypto
    // module if available.
    //
    function autoseed() {
      try {
        if (nodecrypto) {
          return tostring(nodecrypto.randomBytes(width));
        }
        const out = new Uint8Array(width);
        (global.crypto || global.msCrypto).getRandomValues(out);
        return tostring(out);
      } catch (e) {
        const browser = global.navigator;
        const plugins = browser && browser.plugins;
        return [+new Date(), global, plugins, global.screen, tostring(pool)];
      }
    }

    //
    // tostring()
    // Converts an array of charcodes to a string
    //
    function tostring(a) {
      return String.fromCharCode.apply(0, a);
    }

    //
    // When seedrandom.js is loaded, we immediately mix a few bits
    // from the built-in RNG into the entropy pool.  Because we do
    // not want to interfere with deterministic PRNG state later,
    // seedrandom will not call math.random on its own again after
    // initialization.
    //
    mixkey(math.random(), pool);

    //
    // Nodejs and AMD support: export the implementation as a module using
    // either convention.
    //

    // End anonymous scope, and pass initial values.
  })(
      [],      // pool: entropy pool starts empty
      BMMath,  // math: package containing random, pow, and seedrandom
  );
  const BezierFactory = (function() {
    const ob = {};
    ob.getBezierEasing = getBezierEasing;
    const beziers = {};

    function getBezierEasing(a,b,c,d,nm){
      const str =
          nm || ('bez_' + a + '_' + b + '_' + c + '_' + d).replace(/\./g, 'p');
      if (beziers[str]) {
        return beziers[str];
      }
      const bezEasing = new BezierEasing([a, b, c, d]);
      beziers[str] = bezEasing;
      return bezEasing;
    }

    // These values are established by empiricism with tests (tradeoff:
    // performance VS precision)
    const NEWTON_ITERATIONS = 4;
    const NEWTON_MIN_SLOPE = 0.001;
    const SUBDIVISION_PRECISION = 0.0000001;
    const SUBDIVISION_MAX_ITERATIONS = 10;

    const kSplineTableSize = 11;
    const kSampleStepSize = 1.0 / (kSplineTableSize - 1.0);

    const float32ArraySupported = typeof Float32Array === 'function';

    function A(aA1, aA2) {
      return 1.0 - 3.0 * aA2 + 3.0 * aA1;
    }
    function B(aA1, aA2) {
      return 3.0 * aA2 - 6.0 * aA1;
    }
    function C(aA1) {
      return 3.0 * aA1;
    }

    // Returns x(t) given t, x1, and x2, or y(t) given t, y1, and y2.
    function calcBezier (aT, aA1, aA2) {
      return ((A(aA1, aA2) * aT + B(aA1, aA2)) * aT + C(aA1)) * aT;
    }

    // Returns dx/dt given t, x1, and x2, or dy/dt given t, y1, and y2.
    function getSlope (aT, aA1, aA2) {
      return 3.0 * A(aA1, aA2) * aT * aT + 2.0 * B(aA1, aA2) * aT + C(aA1);
    }

    function binarySubdivide (aX, aA, aB, mX1, mX2) {
      let currentX;
      let currentT;
      let i = 0;
      do {
        currentT = aA + (aB - aA) / 2.0;
        currentX = calcBezier(currentT, mX1, mX2) - aX;
        if (currentX > 0.0) {
          aB = currentT;
        } else {
          aA = currentT;
        }
      } while (Math.abs(currentX) > SUBDIVISION_PRECISION &&
               ++i < SUBDIVISION_MAX_ITERATIONS);
      return currentT;
    }

    function newtonRaphsonIterate (aX, aGuessT, mX1, mX2) {
      for (let i = 0; i < NEWTON_ITERATIONS; ++i) {
        const currentSlope = getSlope(aGuessT, mX1, mX2);
        if (currentSlope === 0.0) {
          return aGuessT;
        }
        const currentX = calcBezier(aGuessT, mX1, mX2) - aX;
        aGuessT -= currentX / currentSlope;
      }
      return aGuessT;
    }

    /**
     * points is an array of [ mX1, mY1, mX2, mY2 ]
     */
    function BezierEasing (points) {
      this._p = points;
      this._mSampleValues = float32ArraySupported ?
          new Float32Array(kSplineTableSize) :
          new Array(kSplineTableSize);
      this._precomputed = false;

      this.get = this.get.bind(this);
    }

    BezierEasing.prototype = {

      get: function(x) {
        const mX1 = this._p[0];
        const mY1 = this._p[1];
        const mX2 = this._p[2];
        const mY2 = this._p[3];
        if (!this._precomputed) {
          this._precompute();
        }
        if (mX1 === mY1 && mX2 === mY2) {
          return x;
        }  // linear
        // Because JavaScript number are imprecise, we should guarantee the
        // extremes are right.
        if (x === 0) {
          return 0;
        }
        if (x === 1) {
          return 1;
        }
        return calcBezier(this._getTForX(x), mY1, mY2);
      },

      // Private part

      _precompute: function() {
        const mX1 = this._p[0];
        const mY1 = this._p[1];
        const mX2 = this._p[2];
        const mY2 = this._p[3];
        this._precomputed = true;
        if (mX1 !== mY1 || mX2 !== mY2) {
          this._calcSampleValues();
        }
      },

      _calcSampleValues: function() {
        const mX1 = this._p[0];
        const mX2 = this._p[2];
        for (let i = 0; i < kSplineTableSize; ++i) {
          this._mSampleValues[i] = calcBezier(i * kSampleStepSize, mX1, mX2);
        }
      },

      /**
       * getTForX chose the fastest heuristic to determine the percentage value
       * precisely from a given X projection.
       */
      _getTForX: function(aX) {
        const mX1 = this._p[0];
        const mX2 = this._p[2];
        const mSampleValues = this._mSampleValues;

        let intervalStart = 0.0;
        let currentSample = 1;
        const lastSample = kSplineTableSize - 1;

        for (;
             currentSample !== lastSample && mSampleValues[currentSample] <= aX;
             ++currentSample) {
          intervalStart += kSampleStepSize;
        }
        --currentSample;

        // Interpolate to provide an initial guess for t
        const dist = (aX - mSampleValues[currentSample]) /
            (mSampleValues[currentSample + 1] - mSampleValues[currentSample]);
        const guessForT = intervalStart + dist * kSampleStepSize;

        const initialSlope = getSlope(guessForT, mX1, mX2);
        if (initialSlope >= NEWTON_MIN_SLOPE) {
          return newtonRaphsonIterate(aX, guessForT, mX1, mX2);
        } else if (initialSlope === 0.0) {
          return guessForT;
        } else {
          return binarySubdivide(
              aX, intervalStart, intervalStart + kSampleStepSize, mX1, mX2);
        }
      },
    };

    return ob;
  }());
  (function() {
    let lastTime = 0;
    const vendors = ['ms', 'moz', 'webkit', 'o'];
    for (let x = 0; x < vendors.length && !window.requestAnimationFrame; ++x) {
      window.requestAnimationFrame =
          window[vendors[x] + 'RequestAnimationFrame'];
      window.cancelAnimationFrame =
          window[vendors[x] + 'CancelAnimationFrame'] ||
          window[vendors[x] + 'CancelRequestAnimationFrame'];
    }
    if (!window.requestAnimationFrame) {
      window.requestAnimationFrame = function(callback, element) {
        const currTime = new Date().getTime();
        const timeToCall = Math.max(0, 16 - (currTime - lastTime));
        const id = setTimeout(function() {
          callback(currTime + timeToCall);
        }, timeToCall);
        lastTime = currTime + timeToCall;
        return id;
      };
    }
    if (!window.cancelAnimationFrame) {
      window.cancelAnimationFrame = function(id) {
        clearTimeout(id);
      };
    }
  }());

  function extendPrototype(sources, destination) {
    let i;
    const len = sources.length;
    let sourcePrototype;
    for (i = 0;i < len;i += 1) {
      sourcePrototype = sources[i].prototype;
      for (const attr in sourcePrototype) {
        if (sourcePrototype.hasOwnProperty(attr)) {
          destination.prototype[attr] = sourcePrototype[attr];
        }
      }
    }
  }

  function getDescriptor(object, prop) {
    return Object.getOwnPropertyDescriptor(object, prop);
  }

  function createProxyFunction(prototype) {
    function ProxyFunction() {}
    ProxyFunction.prototype = prototype;
    return ProxyFunction;
  }
  function bezFunction() {
    const easingFunctions = [];
    const math = Math;

    function pointOnLine2D(x1,y1, x2,y2, x3,y3){
      const det1 =
          (x1 * y2) + (y1 * x3) + (x2 * y3) - (x3 * y2) - (y3 * x1) - (x2 * y1);
      return det1 > -0.001 && det1 < 0.001;
    }

    function pointOnLine3D(x1,y1,z1, x2,y2,z2, x3,y3,z3){
      if (z1 === 0 && z2 === 0 && z3 === 0) {
        return pointOnLine2D(x1, y1, x2, y2, x3, y3);
      }
      const dist1 = Math.sqrt(
          Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2) + Math.pow(z2 - z1, 2));
      const dist2 = Math.sqrt(
          Math.pow(x3 - x1, 2) + Math.pow(y3 - y1, 2) + Math.pow(z3 - z1, 2));
      const dist3 = Math.sqrt(
          Math.pow(x3 - x2, 2) + Math.pow(y3 - y2, 2) + Math.pow(z3 - z2, 2));
      let diffDist;
      if (dist1 > dist2) {
        if (dist1 > dist3) {
          diffDist = dist1 - dist2 - dist3;
        } else {
          diffDist = dist3 - dist2 - dist1;
        }
      } else if (dist3 > dist2) {
        diffDist = dist3 - dist2 - dist1;
      } else {
        diffDist = dist2 - dist1 - dist3;
      }
      return diffDist > -0.0001 && diffDist < 0.0001;
    }

    const getBezierLength = (function() {
      return function(pt1, pt2, pt3, pt4) {
        const curveSegments = defaultCurveSegments;
        let k;
        let i;
        let ptCoord;
        let perc;
        let addedLength = 0;
        let ptDistance;
        const point = [];
        const lastPoint = [];
        const lengthData = bezier_length_pool.newElement();
        const len = pt3.length;
        for (k = 0; k < curveSegments; k += 1) {
          perc = k / (curveSegments - 1);
          ptDistance = 0;
          for (i = 0; i < len; i += 1) {
            ptCoord = bm_pow(1 - perc, 3) * pt1[i] +
                3 * bm_pow(1 - perc, 2) * perc * pt3[i] +
                3 * (1 - perc) * bm_pow(perc, 2) * pt4[i] +
                bm_pow(perc, 3) * pt2[i];
            point[i] = ptCoord;
            if (lastPoint[i] !== null) {
              ptDistance += bm_pow(point[i] - lastPoint[i], 2);
            }
            lastPoint[i] = point[i];
          }
          if (ptDistance) {
            ptDistance = bm_sqrt(ptDistance);
            addedLength += ptDistance;
          }
          lengthData.percents[k] = perc;
          lengthData.lengths[k] = addedLength;
        }
        lengthData.addedLength = addedLength;
        return lengthData;
      };
    }());

    function getSegmentsLength(shapeData) {
      const segmentsLength = segments_length_pool.newElement();
      const closed = shapeData.c;
      const pathV = shapeData.v;
      const pathO = shapeData.o;
      const pathI = shapeData.i;
      let i;
      const len = shapeData._length;
      const lengths = segmentsLength.lengths;
      let totalLength = 0;
      for (i = 0; i < len - 1; i += 1) {
        lengths[i] =
            getBezierLength(pathV[i], pathV[i + 1], pathO[i], pathI[i + 1]);
        totalLength += lengths[i].addedLength;
      }
      if (closed && len) {
        lengths[i] = getBezierLength(pathV[i], pathV[0], pathO[i], pathI[0]);
        totalLength += lengths[i].addedLength;
      }
      segmentsLength.totalLength = totalLength;
      return segmentsLength;
    }

    function BezierData(length){
      this.segmentLength = 0;
      this.points = new Array(length);
    }

    function PointData(partial,point){
      this.partialLength = partial;
      this.point = point;
    }

    const buildBezierData = (function() {
      const storedData = {};

      return function(pt1, pt2, pt3, pt4) {
        const bezierName =
            (pt1[0] + '_' + pt1[1] + '_' + pt2[0] + '_' + pt2[1] + '_' +
             pt3[0] + '_' + pt3[1] + '_' + pt4[0] + '_' + pt4[1])
                .replace(/\./g, 'p');
        if (!storedData[bezierName]) {
          let curveSegments = defaultCurveSegments;
          let k;
          let i;
          let ptCoord;
          let perc;
          let addedLength = 0;
          let ptDistance;
          let point;
          let lastPoint = null;
          if (pt1.length === 2 && (pt1[0] != pt2[0] || pt1[1] != pt2[1]) &&
              pointOnLine2D(
                  pt1[0], pt1[1], pt2[0], pt2[1], pt1[0] + pt3[0],
                  pt1[1] + pt3[1]) &&
              pointOnLine2D(
                  pt1[0], pt1[1], pt2[0], pt2[1], pt2[0] + pt4[0],
                  pt2[1] + pt4[1])) {
            curveSegments = 2;
          }
          const bezierData = new BezierData(curveSegments);
          const len = pt3.length;
          for (k = 0; k < curveSegments; k += 1) {
            point = createSizedArray(len);
            perc = k / (curveSegments - 1);
            ptDistance = 0;
            for (i = 0; i < len; i += 1) {
              ptCoord = bm_pow(1 - perc, 3) * pt1[i] +
                  3 * bm_pow(1 - perc, 2) * perc * (pt1[i] + pt3[i]) +
                  3 * (1 - perc) * bm_pow(perc, 2) * (pt2[i] + pt4[i]) +
                  bm_pow(perc, 3) * pt2[i];
              point[i] = ptCoord;
              if (lastPoint !== null) {
                ptDistance += bm_pow(point[i] - lastPoint[i], 2);
              }
            }
            ptDistance = bm_sqrt(ptDistance);
            addedLength += ptDistance;
            bezierData.points[k] = new PointData(ptDistance, point);
            lastPoint = point;
          }
          bezierData.segmentLength = addedLength;
          storedData[bezierName] = bezierData;
        }
        return storedData[bezierName];
      };
    }());

    function getDistancePerc(perc,bezierData){
      const percents = bezierData.percents;
      const lengths = bezierData.lengths;
      const len = percents.length;
      let initPos = bm_floor((len - 1) * perc);
      const lengthPos = perc * bezierData.addedLength;
      let lPerc = 0;
      if (initPos === len - 1 || initPos === 0 ||
          lengthPos === lengths[initPos]) {
        return percents[initPos];
      } else {
        const dir = lengths[initPos] > lengthPos ? -1 : 1;
        let flag = true;
        while (flag) {
          if (lengths[initPos] <= lengthPos &&
              lengths[initPos + 1] > lengthPos) {
            lPerc = (lengthPos - lengths[initPos]) /
                (lengths[initPos + 1] - lengths[initPos]);
            flag = false;
          } else {
            initPos += dir;
          }
          if (initPos < 0 || initPos >= len - 1) {
            // FIX for TypedArrays that don't store floating point values with
            // enough accuracy
            if (initPos === len - 1) {
              return percents[initPos];
            }
            flag = false;
          }
        }
        return percents[initPos] +
            (percents[initPos + 1] - percents[initPos]) * lPerc;
      }
    }

    function getPointInSegment(pt1, pt2, pt3, pt4, percent, bezierData) {
      const t1 = getDistancePerc(percent, bezierData);
      const u0 = 1;
      const u1 = 1 - t1;
      const ptX = Math.round(
                      (u1 * u1 * u1 * pt1[0] +
                       (t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * pt3[0] +
                       (t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * pt4[0] +
                       t1 * t1 * t1 * pt2[0]) *
                      1000) /
          1000;
      const ptY = Math.round(
                      (u1 * u1 * u1 * pt1[1] +
                       (t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * pt3[1] +
                       (t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * pt4[1] +
                       t1 * t1 * t1 * pt2[1]) *
                      1000) /
          1000;
      return [ptX, ptY];
    }

    function getSegmentArray() {

    }

    const bezier_segment_points = createTypedArray('float32', 8);

    function getNewSegment(pt1,pt2,pt3,pt4,startPerc,endPerc, bezierData){
      startPerc = startPerc < 0 ? 0 : startPerc > 1 ? 1 : startPerc;
      const t0 = getDistancePerc(startPerc, bezierData);
      endPerc = endPerc > 1 ? 1 : endPerc;
      const t1 = getDistancePerc(endPerc, bezierData);
      let i;
      const len = pt1.length;
      const u0 = 1 - t0;
      const u1 = 1 - t1;
      const u0u0u0 = u0 * u0 * u0;
      const t0u0u0_3 = t0 * u0 * u0 * 3;
      const t0t0u0_3 = t0 * t0 * u0 * 3;
      const t0t0t0 = t0 * t0 * t0;
      //
      const u0u0u1 = u0 * u0 * u1;
      const t0u0u1_3 = t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1;
      const t0t0u1_3 = t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1;
      const t0t0t1 = t0 * t0 * t1;
      //
      const u0u1u1 = u0 * u1 * u1;
      const t0u1u1_3 = t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1;
      const t0t1u1_3 = t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1;
      const t0t1t1 = t0 * t1 * t1;
      //
      const u1u1u1 = u1 * u1 * u1;
      const t1u1u1_3 = t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1;
      const t1t1u1_3 = t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1;
      const t1t1t1 = t1 * t1 * t1;
      for (i = 0; i < len; i += 1) {
        bezier_segment_points[i * 4] =
            Math.round(
                (u0u0u0 * pt1[i] + t0u0u0_3 * pt3[i] + t0t0u0_3 * pt4[i] +
                 t0t0t0 * pt2[i]) *
                1000) /
            1000;
        bezier_segment_points[i * 4 + 1] =
            Math.round(
                (u0u0u1 * pt1[i] + t0u0u1_3 * pt3[i] + t0t0u1_3 * pt4[i] +
                 t0t0t1 * pt2[i]) *
                1000) /
            1000;
        bezier_segment_points[i * 4 + 2] =
            Math.round(
                (u0u1u1 * pt1[i] + t0u1u1_3 * pt3[i] + t0t1u1_3 * pt4[i] +
                 t0t1t1 * pt2[i]) *
                1000) /
            1000;
        bezier_segment_points[i * 4 + 3] =
            Math.round(
                (u1u1u1 * pt1[i] + t1u1u1_3 * pt3[i] + t1t1u1_3 * pt4[i] +
                 t1t1t1 * pt2[i]) *
                1000) /
            1000;
      }

      return bezier_segment_points;
    }

    return {
      getSegmentsLength: getSegmentsLength,
      getNewSegment: getNewSegment,
      getPointInSegment: getPointInSegment,
      buildBezierData: buildBezierData,
      pointOnLine2D: pointOnLine2D,
      pointOnLine3D: pointOnLine3D,
    };
  }

  const bez = bezFunction();
  function dataFunctionManager() {
    // let tCanvasHelper = createTag('canvas').getContext('2d');

    function completeLayers(layers, comps, fontManager){
      let layerData;
      let animArray;
      let lastFrame;
      let i;
      const len = layers.length;
      let j;
      let jLen;
      let k;
      for (i = 0; i < len; i += 1) {
        layerData = layers[i];
        if (!('ks' in layerData) || layerData.completed) {
          continue;
        }
        layerData.completed = true;
        if (layerData.tt) {
          layers[i - 1].td = layerData.tt;
        }
        animArray = [];
        lastFrame = -1;
        if (layerData.hasMask) {
          const maskProps = layerData.masksProperties;
          jLen = maskProps.length;
          for (j = 0; j < jLen; j += 1) {
            if (maskProps[j].pt.k.i) {
              convertPathsToAbsoluteValues(maskProps[j].pt.k);
            } else {
              const kLen = maskProps[j].pt.k.length;
              for (k = 0; k < kLen; k += 1) {
                if (maskProps[j].pt.k[k].s) {
                  convertPathsToAbsoluteValues(maskProps[j].pt.k[k].s[0]);
                }
                if (maskProps[j].pt.k[k].e) {
                  convertPathsToAbsoluteValues(maskProps[j].pt.k[k].e[0]);
                }
              }
            }
          }
        }
        if (layerData.ty === 0) {
          layerData.layers = findCompLayers(layerData.refId, comps);
          completeLayers(layerData.layers, comps, fontManager);
        } else if (layerData.ty === 4) {
          completeShapes(layerData.shapes);
        } else if (layerData.ty == 5) {
          completeText(layerData, fontManager);
        }
      }
    }

    function findCompLayers(id,comps){
      let i = 0;
      const len = comps.length;
      while (i < len) {
        if (comps[i].id === id) {
          if (!comps[i].layers.__used) {
            comps[i].layers.__used = true;
            return comps[i].layers;
          }
          // eslint-disable-next-line no-restricted-syntax
          return JSON.parse(JSON.stringify(comps[i].layers));
        }
        i += 1;
      }
    }

    function completeShapes(arr){
      let i;
      const len = arr.length;
      let j;
      let jLen;
      let hasPaths = false;
      for (i = len - 1; i >= 0; i -= 1) {
        if (arr[i].ty == 'sh') {
          if (arr[i].ks.k.i) {
            convertPathsToAbsoluteValues(arr[i].ks.k);
          } else {
            jLen = arr[i].ks.k.length;
            for (j = 0; j < jLen; j += 1) {
              if (arr[i].ks.k[j].s) {
                convertPathsToAbsoluteValues(arr[i].ks.k[j].s[0]);
              }
              if (arr[i].ks.k[j].e) {
                convertPathsToAbsoluteValues(arr[i].ks.k[j].e[0]);
              }
            }
          }
          hasPaths = true;
        } else if (arr[i].ty == 'gr') {
          completeShapes(arr[i].it);
        }
      }
      /*if(hasPaths){
          //mx: distance
          //ss: sensitivity
          //dc: decay
          arr.splice(arr.length-1,0,{
              "ty": "ms",
              "mx":20,
              "ss":10,
               "dc":0.001,
              "maxDist":200
          });
      }*/
    }

    function convertPathsToAbsoluteValues(path){
      let i;
      const len = path.i.length;
      for (i = 0; i < len; i += 1) {
        path.i[i][0] += path.v[i][0];
        path.i[i][1] += path.v[i][1];
        path.o[i][0] += path.v[i][0];
        path.o[i][1] += path.v[i][1];
      }
    }

    function checkVersion(minimum,animVersionString){
      const animVersion =
          animVersionString ? animVersionString.split('.') : [100, 100, 100];
      if (minimum[0] > animVersion[0]) {
        return true;
      } else if (animVersion[0] > minimum[0]) {
        return false;
      }
      if (minimum[1] > animVersion[1]) {
        return true;
      } else if (animVersion[1] > minimum[1]) {
        return false;
      }
      if (minimum[2] > animVersion[2]) {
        return true;
      } else if (animVersion[2] > minimum[2]) {
        return false;
      }
    }

    const checkText = (function() {
      const minimumVersion = [4, 4, 14];

      function updateTextLayer(textLayer) {
        const documentData = textLayer.t.d;
        textLayer.t.d = {
          k: [
            {
              s: documentData,
              t: 0,
            },
          ],
        };
      }

      function iterateLayers(layers) {
        let i;
        const len = layers.length;
        for (i = 0; i < len; i += 1) {
          if (layers[i].ty === 5) {
            updateTextLayer(layers[i]);
          }
        }
      }

      return function(animationData) {
        if (checkVersion(minimumVersion, animationData.v)) {
          iterateLayers(animationData.layers);
          if (animationData.assets) {
            let i;
            const len = animationData.assets.length;
            for (i = 0; i < len; i += 1) {
              if (animationData.assets[i].layers) {
                iterateLayers(animationData.assets[i].layers);
              }
            }
          }
        }
      };
    }());

    const checkChars =
        (function() {
          const minimumVersion = [4, 7, 99];
          return function(animationData) {
            if(animationData.chars && !checkVersion(minimumVersion,animationData.v)){
              let i;
              const len = animationData.chars.length;
              let j;
              let jLen;
              let k;
              let pathData;
              let paths;
              for (i = 0; i < len; i += 1) {
                if (animationData.chars[i].data &&
                    animationData.chars[i].data.shapes) {
                  paths = animationData.chars[i].data.shapes[0].it;
                  jLen = paths.length;

                  for (j = 0; j < jLen; j += 1) {
                    pathData = paths[j].ks.k;
                    if (!pathData.__converted) {
                      convertPathsToAbsoluteValues(paths[j].ks.k);
                      pathData.__converted = true;
                    }
                  }
                }
              }
            }
          };
        }());

    const checkColors = (function() {
      const minimumVersion = [4, 1, 9];

      function iterateShapes(shapes) {
        let i;
        const len = shapes.length;
        let j;
        let jLen;
        for (i = 0; i < len; i += 1) {
          if (shapes[i].ty === 'gr') {
            iterateShapes(shapes[i].it);
          } else if (shapes[i].ty === 'fl' || shapes[i].ty === 'st') {
            if (shapes[i].c.k && shapes[i].c.k[0].i) {
              jLen = shapes[i].c.k.length;
              for (j = 0; j < jLen; j += 1) {
                if (shapes[i].c.k[j].s) {
                  shapes[i].c.k[j].s[0] /= 255;
                  shapes[i].c.k[j].s[1] /= 255;
                  shapes[i].c.k[j].s[2] /= 255;
                  shapes[i].c.k[j].s[3] /= 255;
                }
                if (shapes[i].c.k[j].e) {
                  shapes[i].c.k[j].e[0] /= 255;
                  shapes[i].c.k[j].e[1] /= 255;
                  shapes[i].c.k[j].e[2] /= 255;
                  shapes[i].c.k[j].e[3] /= 255;
                }
              }
            } else {
              shapes[i].c.k[0] /= 255;
              shapes[i].c.k[1] /= 255;
              shapes[i].c.k[2] /= 255;
              shapes[i].c.k[3] /= 255;
            }
          }
        }
      }

      function iterateLayers(layers) {
        let i;
        const len = layers.length;
        for (i = 0; i < len; i += 1) {
          if (layers[i].ty === 4) {
            iterateShapes(layers[i].shapes);
          }
        }
      }

      return function(animationData) {
        if (checkVersion(minimumVersion, animationData.v)) {
          iterateLayers(animationData.layers);
          if (animationData.assets) {
            let i;
            const len = animationData.assets.length;
            for (i = 0; i < len; i += 1) {
              if (animationData.assets[i].layers) {
                iterateLayers(animationData.assets[i].layers);
              }
            }
          }
        }
      };
    }());

    const checkShapes = (function() {
      const minimumVersion = [4, 4, 18];



      function completeShapes(arr) {
        let i;
        const len = arr.length;
        let j;
        let jLen;
        let hasPaths = false;
        for (i = len - 1; i >= 0; i -= 1) {
          if (arr[i].ty == 'sh') {
            if (arr[i].ks.k.i) {
              arr[i].ks.k.c = arr[i].closed;
            } else {
              jLen = arr[i].ks.k.length;
              for (j = 0; j < jLen; j += 1) {
                if (arr[i].ks.k[j].s) {
                  arr[i].ks.k[j].s[0].c = arr[i].closed;
                }
                if (arr[i].ks.k[j].e) {
                  arr[i].ks.k[j].e[0].c = arr[i].closed;
                }
              }
            }
            hasPaths = true;
          } else if (arr[i].ty == 'gr') {
            completeShapes(arr[i].it);
          }
        }
      }

      function iterateLayers(layers) {
        let layerData;
        let i;
        const len = layers.length;
        let j;
        let jLen;
        let k;
        for (i = 0; i < len; i += 1) {
          layerData = layers[i];
          if (layerData.hasMask) {
            const maskProps = layerData.masksProperties;
            jLen = maskProps.length;
            for (j = 0; j < jLen; j += 1) {
              if (maskProps[j].pt.k.i) {
                maskProps[j].pt.k.c = maskProps[j].cl;
              } else {
                const kLen = maskProps[j].pt.k.length;
                for (k = 0; k < kLen; k += 1) {
                  if (maskProps[j].pt.k[k].s) {
                    maskProps[j].pt.k[k].s[0].c = maskProps[j].cl;
                  }
                  if (maskProps[j].pt.k[k].e) {
                    maskProps[j].pt.k[k].e[0].c = maskProps[j].cl;
                  }
                }
              }
            }
          }
          if (layerData.ty === 4) {
            completeShapes(layerData.shapes);
          }
        }
      }

      return function(animationData) {
        if (checkVersion(minimumVersion, animationData.v)) {
          iterateLayers(animationData.layers);
          if (animationData.assets) {
            let i;
            const len = animationData.assets.length;
            for (i = 0; i < len; i += 1) {
              if (animationData.assets[i].layers) {
                iterateLayers(animationData.assets[i].layers);
              }
            }
          }
        }
      };
    }());

    function completeData(animationData, fontManager){
      if (animationData.__complete) {
        return;
      }
      checkColors(animationData);
      checkText(animationData);
      checkChars(animationData);
      checkShapes(animationData);
      completeLayers(animationData.layers, animationData.assets, fontManager);
      animationData.__complete = true;
      // blitAnimation(animationData, animationData.assets, fontManager);
    }

    function completeText(data, fontManager){
      if (data.t.a.length === 0 && !('m' in data.t.p)) {
        data.singleShape = true;
      }
    }

    const moduleOb = {};
    moduleOb.completeData = completeData;
    moduleOb.checkColors = checkColors;
    moduleOb.checkChars = checkChars;
    moduleOb.checkShapes = checkShapes;
    moduleOb.completeLayers = completeLayers;

    return moduleOb;
  }

  const dataManager = dataFunctionManager();

  dataManager.completeData = function(animationData, fontManager) {
    if(animationData.__complete){
      return;
    }
    this.checkColors(animationData);
    this.checkChars(animationData);
    this.checkShapes(animationData);
    this.completeLayers(
        animationData.layers, animationData.assets, fontManager);
    animationData.__complete = true;
  };

  let FontManager = (function() {
    const maxWaitingTime = 5000;
    const emptyChar = {
      w: 0,
      size: 0,
      shapes: [],
    };
    let combinedCharacters = [];
    //Hindi characters
    combinedCharacters = combinedCharacters.concat([2304, 2305, 2306, 2307, 2362, 2363, 2364, 2364, 2366
    , 2367, 2368, 2369, 2370, 2371, 2372, 2373, 2374, 2375, 2376, 2377, 2378, 2379
    , 2380, 2381, 2382, 2383, 2387, 2388, 2389, 2390, 2391, 2402, 2403]);

    function setUpNode(font, family){
      const parentNode = createTag('span');
      parentNode.style.fontFamily = family;
      const node = createTag('span');
      // Characters that vary significantly among different fonts
      node.innerHTML = 'giItT1WQy@!-/#';
      // Visible - so we can measure it - but not on the screen
      parentNode.style.position = 'absolute';
      parentNode.style.left = '-10000px';
      parentNode.style.top = '-10000px';
      // Large font size makes even subtle changes obvious
      parentNode.style.fontSize = '300px';
      // Reset any font properties
      parentNode.style.fontVariant = 'normal';
      parentNode.style.fontStyle = 'normal';
      parentNode.style.fontWeight = 'normal';
      parentNode.style.letterSpacing = '0';
      parentNode.appendChild(node);
      document.body.appendChild(parentNode);

      // Remember width with no applied web font
      const width = node.offsetWidth;
      node.style.fontFamily = font + ', ' + family;
      return {node: node, w: width, parent: parentNode};
    }

    function checkLoadedFonts() {
      let i;
      const len = this.fonts.length;
      let node;
      let w;
      let loadedCount = len;
      for (i = 0; i < len; i += 1) {
        if (this.fonts[i].loaded) {
          loadedCount -= 1;
          continue;
        }
        if (this.fonts[i].fOrigin === 'n' || this.fonts[i].origin === 0) {
          this.fonts[i].loaded = true;
        } else {
          node = this.fonts[i].monoCase.node;
          w = this.fonts[i].monoCase.w;
          if (node.offsetWidth !== w) {
            loadedCount -= 1;
            this.fonts[i].loaded = true;
          } else {
            node = this.fonts[i].sansCase.node;
            w = this.fonts[i].sansCase.w;
            if (node.offsetWidth !== w) {
              loadedCount -= 1;
              this.fonts[i].loaded = true;
            }
          }
          if (this.fonts[i].loaded) {
            this.fonts[i].sansCase.parent.parentNode.removeChild(
                this.fonts[i].sansCase.parent);
            this.fonts[i].monoCase.parent.parentNode.removeChild(
                this.fonts[i].monoCase.parent);
          }
        }
      }

      if (loadedCount !== 0 && Date.now() - this.initTime < maxWaitingTime) {
        setTimeout(this.checkLoadedFonts.bind(this), 20);
      } else {
        setTimeout(function() {
          this.isLoaded = true;
        }.bind(this), 0);
      }
    }

    function createHelper(def, fontData){
      const tHelper = createNS('text');
      tHelper.style.fontSize = '100px';
      // tHelper.style.fontFamily = fontData.fFamily;
      tHelper.setAttribute('font-family', fontData.fFamily);
      tHelper.setAttribute('font-style', fontData.fStyle);
      tHelper.setAttribute('font-weight', fontData.fWeight);
      tHelper.textContent = '1';
      if (fontData.fClass) {
        tHelper.style.fontFamily = 'inherit';
        tHelper.setAttribute('class', fontData.fClass);
      } else {
        tHelper.style.fontFamily = fontData.fFamily;
      }
      def.appendChild(tHelper);
      const tCanvasHelper = createTag('canvas').getContext('2d');
      tCanvasHelper.font = fontData.fWeight + ' ' + fontData.fStyle +
          ' 100px ' + fontData.fFamily;
      // tCanvasHelper.font = ' 100px '+ fontData.fFamily;
      return tHelper;
    }

    function addFonts(fontData, defs){
      if (!fontData) {
        this.isLoaded = true;
        return;
      }
      if (this.chars) {
        this.isLoaded = true;
        this.fonts = fontData.list;
        return;
      }


      const fontArr = fontData.list;
      let i;
      const len = fontArr.length;
      let _pendingFonts = len;
      for (i = 0; i < len; i += 1) {
        let shouldLoadFont = true;
        fontArr[i].loaded = false;
        fontArr[i].monoCase = setUpNode(fontArr[i].fFamily, 'monospace');
        fontArr[i].sansCase = setUpNode(fontArr[i].fFamily, 'sans-serif');
        if (!fontArr[i].fPath) {
          fontArr[i].loaded = true;
          _pendingFonts -= 1;
        } else if (fontArr[i].fOrigin === 'p' || fontArr[i].origin === 3) {
          const loadedSelector = document.querySelectorAll(
              'style[f-forigin="p"][f-family="' + fontArr[i].fFamily +
              '"], style[f-origin="3"][f-family="' + fontArr[i].fFamily + '"]');

          if (loadedSelector.length > 0) {
            shouldLoadFont = false;
          }

          if (shouldLoadFont) {
            const s = createTag('style');
            s.setAttribute('f-forigin', fontArr[i].fOrigin);
            s.setAttribute('f-origin', fontArr[i].origin);
            s.setAttribute('f-family', fontArr[i].fFamily);
            s.type = 'text/css';
            s.innerHTML = '@font-face {' +
                'font-family: ' + fontArr[i].fFamily +
                '; font-style: normal; src: url(\'' + fontArr[i].fPath +
                '\');}';
            defs.appendChild(s);
          }
        } else if (fontArr[i].fOrigin === 'g' || fontArr[i].origin === 1) {
          const loadedSelector = document.querySelectorAll(
              'link[f-forigin="g"], link[f-origin="1"]');

          for (let j = 0; j < loadedSelector.length; j++) {
            if (loadedSelector[j].href.indexOf(fontArr[i].fPath) !== -1) {
              // Font is already loaded
              shouldLoadFont = false;
            }
          }

          if (shouldLoadFont) {
            const l = createTag('link');
            l.setAttribute('f-forigin', fontArr[i].fOrigin);
            l.setAttribute('f-origin', fontArr[i].origin);
            l.type = 'text/css';
            l.rel = 'stylesheet';
            l.href = fontArr[i].fPath;
            document.body.appendChild(l);
          }
        } else if (fontArr[i].fOrigin === 't' || fontArr[i].origin === 2) {
          const loadedSelector = document.querySelectorAll(
              'script[f-forigin="t"], script[f-origin="2"]');

          for (let j = 0; j < loadedSelector.length; j++) {
            if (fontArr[i].fPath === loadedSelector[j].src) {
              // Font is already loaded
              shouldLoadFont = false;
            }
          }

          if (shouldLoadFont) {
            const sc = createTag('link');
            sc.setAttribute('f-forigin', fontArr[i].fOrigin);
            sc.setAttribute('f-origin', fontArr[i].origin);
            sc.setAttribute('rel', 'stylesheet');
            sc.setAttribute('href', fontArr[i].fPath);
            defs.appendChild(sc);
          }
        }
        fontArr[i].helper = createHelper(defs, fontArr[i]);
        fontArr[i].cache = {};
        this.fonts.push(fontArr[i]);
      }
      if (_pendingFonts === 0) {
        this.isLoaded = true;
      } else {
        // On some cases even if the font is loaded, it won't load correctly
        // when measuring text on canvas. Adding this timeout seems to fix it
        setTimeout(this.checkLoadedFonts.bind(this), 100);
      }
    }

    function addChars(chars){
      if (!chars) {
        return;
      }
      if (!this.chars) {
        this.chars = [];
      }
      let i;
      const len = chars.length;
      let j;
      let jLen = this.chars.length;
      let found;
      for (i = 0; i < len; i += 1) {
        j = 0;
        found = false;
        while (j < jLen) {
          if (this.chars[j].style === chars[i].style &&
              this.chars[j].fFamily === chars[i].fFamily &&
              this.chars[j].ch === chars[i].ch) {
            found = true;
          }
          j += 1;
        }
        if (!found) {
          this.chars.push(chars[i]);
          jLen += 1;
        }
      }
    }

    function getCharData(char, style, font){
      let i = 0;
      const len = this.chars.length;
      while (i < len) {
        if (this.chars[i].ch === char && this.chars[i].style === style &&
            this.chars[i].fFamily === font) {
          return this.chars[i];
        }
        i += 1;
      }
      if (console && console.warn) {
        console.warn(
            'Missing character from exported characters list: ', char, style,
            font);
      }
      return emptyChar;
    }

    function measureText(char, fontName, size) {
      const fontData = this.getFontByName(fontName);
      const index = char.charCodeAt(0);
      if (!fontData.cache[index + 1]) {
        const tHelper = fontData.helper;
        // Canvas version
        // fontData.cache[index] = tHelper.measureText(char).width / 100;
        // SVG version
        // console.log(tHelper.getBBox().width)
        if (char === ' ') {
          tHelper.textContent = '|' + char + '|';
          const doubleSize = tHelper.getComputedTextLength();
          tHelper.textContent = '||';
          const singleSize = tHelper.getComputedTextLength();
          fontData.cache[index + 1] = (doubleSize - singleSize) / 100;
        } else {
          tHelper.textContent = char;
          fontData.cache[index + 1] = (tHelper.getComputedTextLength()) / 100;
        }
      }
      return fontData.cache[index + 1] * size;
    }

    function getFontByName(name){
      let i = 0;
      const len = this.fonts.length;
      while (i < len) {
        if (this.fonts[i].fName === name) {
          return this.fonts[i];
        }
        i += 1;
      }
      return this.fonts[0];
    }

    function getCombinedCharacterCodes() {
      return combinedCharacters;
    }

    function loaded() {
      return this.isLoaded;
    }

    const Font = function() {
      this.fonts = [];
      this.chars = null;
      this.typekitLoaded = 0;
      this.isLoaded = false;
      this.initTime = Date.now();
    };
    //TODO: for now I'm adding these methods to the Class and not the prototype. Think of a better way to implement it.
    Font.getCombinedCharacterCodes = getCombinedCharacterCodes;

    Font.prototype.addChars = addChars;
    Font.prototype.addFonts = addFonts;
    Font.prototype.getCharData = getCharData;
    Font.prototype.getFontByName = getFontByName;
    Font.prototype.measureText = measureText;
    Font.prototype.checkLoadedFonts = checkLoadedFonts;
    Font.prototype.loaded = loaded;

    return Font;
  }());
  FontManager = (function() {
    const Font = function() {
      this.fonts = [];
      this.chars = null;
      this.typekitLoaded = 0;
      this.isLoaded = false;
      this.initTime = Date.now();
    };
    return Font;
  }());

  const
      PropertyFactory =
          (function() {
            const initFrame = initialDefaultFrame;
            const math_abs = Math.abs;

            function interpolateValue(frameNum, caching) {
              const offsetTime = this.offsetTime;
              let newValue;
              if (this.propType === 'multidimensional') {
                newValue = createTypedArray('float32', this.pv.length);
              }
              let iterationIndex = caching.lastIndex;
              let i = iterationIndex;
              let len = this.keyframes.length - 1;
              let flag = true;
              let keyData;
              let nextKeyData;

              while (flag) {
                keyData = this.keyframes[i];
                nextKeyData = this.keyframes[i + 1];
                if (i === len - 1 && frameNum >= nextKeyData.t - offsetTime) {
                  if (keyData.h) {
                    keyData = nextKeyData;
                  }
                  iterationIndex = 0;
                  break;
                }
                if ((nextKeyData.t - offsetTime) > frameNum) {
                  iterationIndex = i;
                  break;
                }
                if (i < len - 1) {
                  i += 1;
                } else {
                  iterationIndex = 0;
                  flag = false;
                }
              }

              let k;
              let perc;
              let jLen;
              let j;
              let fnc;
              const nextKeyTime = nextKeyData.t - offsetTime;
              const keyTime = keyData.t - offsetTime;
              let endValue;
              if (keyData.to) {
                if (!keyData.bezierData) {
                  keyData.bezierData = bez.buildBezierData(
                      keyData.s, nextKeyData.s || keyData.e, keyData.to,
                      keyData.ti);
                }
                const bezierData = keyData.bezierData;
                if (frameNum >= nextKeyTime || frameNum < keyTime) {
                  const ind = frameNum >= nextKeyTime ?
                      bezierData.points.length - 1 :
                      0;
                  const kLen = bezierData.points[ind].point.length;
                  for (k = 0; k < kLen; k += 1) {
                    newValue[k] = bezierData.points[ind].point[k];
                  }
                  // caching._lastKeyframeIndex = -1;
                } else {
                  if (keyData.__fnct) {
                    fnc = keyData.__fnct;
                  } else {
                    fnc = BezierFactory.getBezierEasing(keyData.o.x, keyData.o.y, keyData.i.x, keyData.i.y, keyData.n).get;
                    keyData.__fnct = fnc;
                  }
                  perc = fnc((frameNum - keyTime) / (nextKeyTime - keyTime));
                  const distanceInLine = bezierData.segmentLength * perc;

                  let segmentPerc;
                  let addedLength = (caching.lastFrame < frameNum &&
                                     caching._lastKeyframeIndex === i) ?
                      caching._lastAddedLength :
                      0;
                  j = (caching.lastFrame < frameNum &&
                       caching._lastKeyframeIndex === i) ?
                      caching._lastPoint :
                      0;
                  flag = true;
                  jLen = bezierData.points.length;
                  while (flag) {
                    addedLength += bezierData.points[j].partialLength;
                    if (distanceInLine === 0 || perc === 0 || j === bezierData.points.length - 1) {
                      const kLen = bezierData.points[j].point.length;
                      for (k = 0; k < kLen; k += 1) {
                        newValue[k] = bezierData.points[j].point[k];
                      }
                      break;
                    } else if (distanceInLine >= addedLength && distanceInLine < addedLength + bezierData.points[j + 1].partialLength) {
                      segmentPerc = (distanceInLine - addedLength) /
                          bezierData.points[j + 1].partialLength;
                      const kLen = bezierData.points[j].point.length;
                      for (k = 0; k < kLen; k += 1) {
                        newValue[k] = bezierData.points[j].point[k] +
                            (bezierData.points[j + 1].point[k] -
                             bezierData.points[j].point[k]) *
                                segmentPerc;
                      }
                      break;
                    }
                    if (j < jLen - 1){
                      j += 1;
                    } else {
                      flag = false;
                    }
                  }
                  caching._lastPoint = j;
                  caching._lastAddedLength =
                      addedLength - bezierData.points[j].partialLength;
                  caching._lastKeyframeIndex = i;
                }
              } else {
                let outX;
                let outY;
                let inX;
                let inY;
                let keyValue;
                len = keyData.s.length;
                endValue = nextKeyData.s || keyData.e;
                if (this.sh && keyData.h !== 1) {
                  if (frameNum >= nextKeyTime) {
                    newValue[0] = endValue[0];
                    newValue[1] = endValue[1];
                    newValue[2] = endValue[2];
                  } else if (frameNum <= keyTime) {
                    newValue[0] = keyData.s[0];
                    newValue[1] = keyData.s[1];
                    newValue[2] = keyData.s[2];
                  } else {
                    const quatStart = createQuaternion(keyData.s);
                    const quatEnd = createQuaternion(endValue);
                    const time = (frameNum - keyTime) / (nextKeyTime - keyTime);
                    quaternionToEuler(newValue, slerp(quatStart, quatEnd, time));
                  }
                } else {
                  for (i = 0; i < len; i += 1) {
                    if (keyData.h !== 1) {
                      if (frameNum >= nextKeyTime) {
                        perc = 1;
                      } else if (frameNum < keyTime) {
                        perc = 0;
                      } else {
                        if (keyData.o.x.constructor === Array) {
                          if (!keyData.__fnct) {
                            keyData.__fnct = [];
                          }
                          if (!keyData.__fnct[i]) {
                            outX = (typeof keyData.o.x[i] === 'undefined') ?
                                keyData.o.x[0] :
                                keyData.o.x[i];
                            outY = (typeof keyData.o.y[i] === 'undefined') ?
                                keyData.o.y[0] :
                                keyData.o.y[i];
                            inX = (typeof keyData.i.x[i] === 'undefined') ?
                                keyData.i.x[0] :
                                keyData.i.x[i];
                            inY = (typeof keyData.i.y[i] === 'undefined') ?
                                keyData.i.y[0] :
                                keyData.i.y[i];
                            fnc = BezierFactory
                                      .getBezierEasing(outX, outY, inX, inY)
                                      .get;
                            keyData.__fnct[i] = fnc;
                          } else {
                            fnc = keyData.__fnct[i];
                          }
                        } else {
                          if (!keyData.__fnct) {
                            outX = keyData.o.x;
                            outY = keyData.o.y;
                            inX = keyData.i.x;
                            inY = keyData.i.y;
                            fnc = BezierFactory
                                      .getBezierEasing(outX, outY, inX, inY)
                                      .get;
                            keyData.__fnct = fnc;
                          } else {
                            fnc = keyData.__fnct;
                          }
                        }
                        perc =
                            fnc((frameNum - keyTime) / (nextKeyTime - keyTime));
                      }
                    }

                    endValue = nextKeyData.s || keyData.e;
                    keyValue = keyData.h === 1 ? keyData.s[i] : keyData.s[i] + (endValue[i] - keyData.s[i]) * perc;

                    if (len === 1) {
                      newValue = keyValue;
                    } else {
                      newValue[i] = keyValue;
                    }
                  }
                }
              }
              caching.lastIndex = iterationIndex;
              return newValue;
            }

            // based on @Toji's https://github.com/toji/gl-matrix/
            function slerp(a, b, t) {
              const out = [];
              const ax = a[0];
              const ay = a[1];
              const az = a[2];
              const aw = a[3];
              let bx = b[0];
              let by = b[1];
              let bz = b[2];
              let bw = b[3];

              let omega;
              let cosom;
              let sinom;
              let scale0;
              let scale1;

              cosom = ax * bx + ay * by + az * bz + aw * bw;
              if (cosom < 0.0) {
                cosom = -cosom;
                bx = -bx;
                by = -by;
                bz = -bz;
                bw = -bw;
              }
              if ((1.0 - cosom) > 0.000001) {
                omega = Math.acos(cosom);
                sinom = Math.sin(omega);
                scale0 = Math.sin((1.0 - t) * omega) / sinom;
                scale1 = Math.sin(t * omega) / sinom;
              } else {
                scale0 = 1.0 - t;
                scale1 = t;
              }
              out[0] = scale0 * ax + scale1 * bx;
              out[1] = scale0 * ay + scale1 * by;
              out[2] = scale0 * az + scale1 * bz;
              out[3] = scale0 * aw + scale1 * bw;

              return out;
            }

            function quaternionToEuler(out, quat) {
              const qx = quat[0];
              const qy = quat[1];
              const qz = quat[2];
              const qw = quat[3];
              const heading = Math.atan2(
                  2 * qy * qw - 2 * qx * qz, 1 - 2 * qy * qy - 2 * qz * qz);
              const attitude = Math.asin(2 * qx * qy + 2 * qz * qw);
              const bank = Math.atan2(
                  2 * qx * qw - 2 * qy * qz, 1 - 2 * qx * qx - 2 * qz * qz);
              out[0] = heading / degToRads;
              out[1] = attitude / degToRads;
              out[2] = bank / degToRads;
            }

            function createQuaternion(values) {
              const heading = values[0] * degToRads;
              const attitude = values[1] * degToRads;
              const bank = values[2] * degToRads;
              const c1 = Math.cos(heading / 2);
              const c2 = Math.cos(attitude / 2);
              const c3 = Math.cos(bank / 2);
              const s1 = Math.sin(heading / 2);
              const s2 = Math.sin(attitude / 2);
              const s3 = Math.sin(bank / 2);
              const w = c1 * c2 * c3 - s1 * s2 * s3;
              const x = s1 * s2 * c3 + c1 * c2 * s3;
              const y = s1 * c2 * c3 + c1 * s2 * s3;
              const z = c1 * s2 * c3 - s1 * c2 * s3;

              return [x, y, z, w];
            }

            function getValueAtCurrentTime() {
              const frameNum = this.comp.renderedFrame - this.offsetTime;
              const initTime = this.keyframes[0].t - this.offsetTime;
              const endTime =
                  this.keyframes[this.keyframes.length - 1].t - this.offsetTime;
              if (!(frameNum === this._caching.lastFrame ||
                    (this._caching.lastFrame !== initFrame &&
                     ((this._caching.lastFrame >= endTime &&
                       frameNum >= endTime) ||
                      (this._caching.lastFrame < initTime &&
                       frameNum < initTime))))) {
                if (this._caching.lastFrame >= frameNum) {
                  this._caching._lastKeyframeIndex = -1;
                  this._caching.lastIndex = 0;
                }

                const renderResult =
                    this.interpolateValue(frameNum, this._caching);
                this.pv = renderResult;
              }
              this._caching.lastFrame = frameNum;
              return this.pv;
            }

            function setVValue(val) {
              let multipliedValue;
              if (this.propType === 'unidimensional') {
                multipliedValue = val * this.mult;
                if (math_abs(this.v - multipliedValue) > 0.00001) {
                  this.v = multipliedValue;
                  this._mdf = true;
                }
              } else {
                let i = 0;
                const len = this.v.length;
                while (i < len) {
                  multipliedValue = val[i] * this.mult;
                  if (math_abs(this.v[i] - multipliedValue) > 0.00001) {
                    this.v[i] = multipliedValue;
                    this._mdf = true;
                  }
                  i += 1;
                }
              }
            }

            function processEffectsSequence() {
              if (this.elem.globalData.frameId === this.frameId ||
                  !this.effectsSequence.length) {
                return;
              }
              if (this.lock) {
                this.setVValue(this.pv);
                return;
              }
              this.lock = true;
              this._mdf = this._isFirstFrame;
              let multipliedValue;
              let i;
              const len = this.effectsSequence.length;
              let finalValue = this.kf ? this.pv : this.data.k;
              for (i = 0; i < len; i += 1) {
                finalValue = this.effectsSequence[i](finalValue);
              }
              this.setVValue(finalValue);
              this._isFirstFrame = false;
              this.lock = false;
              this.frameId = this.elem.globalData.frameId;
            }

            function addEffect(effectFunction) {
              this.effectsSequence.push(effectFunction);
              this.container.addDynamicProperty(this);
            }

            function ValueProperty(elem, data, mult, container) {
              this.propType = 'unidimensional';
              this.mult = mult || 1;
              this.data = data;
              this.v = mult ? data.k * mult : data.k;
              this.pv = data.k;
              this._mdf = false;
              this.elem = elem;
              this.container = container;
              this.comp = elem.comp;
              this.k = false;
              this.kf = false;
              this.vel = 0;
              this.effectsSequence = [];
              this._isFirstFrame = true;
              this.getValue = processEffectsSequence;
              this.setVValue = setVValue;
              this.addEffect = addEffect;
            }

            function MultiDimensionalProperty(elem, data, mult, container) {
              this.propType = 'multidimensional';
              this.mult = mult || 1;
              this.data = data;
              this._mdf = false;
              this.elem = elem;
              this.container = container;
              this.comp = elem.comp;
              this.k = false;
              this.kf = false;
              this.frameId = -1;
              let i;
              const len = data.k.length;
              this.v = createTypedArray('float32', len);
              this.pv = createTypedArray('float32', len);
              const arr = createTypedArray('float32', len);
              this.vel = createTypedArray('float32', len);
              for (i = 0; i < len; i += 1) {
                this.v[i] = data.k[i] * this.mult;
                this.pv[i] = data.k[i];
              }
              this._isFirstFrame = true;
              this.effectsSequence = [];
              this.getValue = processEffectsSequence;
              this.setVValue = setVValue;
              this.addEffect = addEffect;
            }

            function KeyframedValueProperty(elem, data, mult, container) {
              this.propType = 'unidimensional';
              this.keyframes = data.k;
              this.offsetTime = elem.data.st;
              this.frameId = -1;
              this._caching = {
                lastFrame: initFrame,
                lastIndex: 0,
                value: 0,
                _lastKeyframeIndex: -1,
              };
              this.k = true;
              this.kf = true;
              this.data = data;
              this.mult = mult || 1;
              this.elem = elem;
              this.container = container;
              this.comp = elem.comp;
              this.v = initFrame;
              this.pv = initFrame;
              this._isFirstFrame = true;
              this.getValue = processEffectsSequence;
              this.setVValue = setVValue;
              this.interpolateValue = interpolateValue;
              this.effectsSequence = [getValueAtCurrentTime.bind(this)];
              this.addEffect = addEffect;
            }

            function KeyframedMultidimensionalProperty(
                elem, data, mult, container) {
              this.propType = 'multidimensional';
              let i;
              const len = data.k.length;
              let s;
              let e;
              let to;
              let ti;
              for (i = 0; i < len - 1; i += 1) {
                if (data.k[i].to && data.k[i].s && data.k[i].e) {
                  s = data.k[i].s;
                  e = data.k[i].e;
                  to = data.k[i].to;
                  ti = data.k[i].ti;
                  if ((s.length === 2 && !(s[0] === e[0] && s[1] === e[1]) &&
                       bez.pointOnLine2D(
                           s[0], s[1], e[0], e[1], s[0] + to[0],
                           s[1] + to[1]) &&
                       bez.pointOnLine2D(
                           s[0], s[1], e[0], e[1], e[0] + ti[0],
                           e[1] + ti[1])) ||
                      (s.length === 3 &&
                       !(s[0] === e[0] && s[1] === e[1] && s[2] === e[2]) &&
                       bez.pointOnLine3D(
                           s[0], s[1], s[2], e[0], e[1], e[2], s[0] + to[0],
                           s[1] + to[1], s[2] + to[2]) &&
                       bez.pointOnLine3D(
                           s[0], s[1], s[2], e[0], e[1], e[2], e[0] + ti[0],
                           e[1] + ti[1], e[2] + ti[2]))) {
                    data.k[i].to = null;
                    data.k[i].ti = null;
                  }
                  if (s[0] === e[0] && s[1] === e[1] && to[0] === 0 &&
                      to[1] === 0 && ti[0] === 0 && ti[1] === 0) {
                    if(s.length === 2 || (s[2] === e[2] && to[2] === 0 && ti[2] === 0)) {
                      data.k[i].to = null;
                      data.k[i].ti = null;
                    }
                  }
                }
              }
              this.effectsSequence = [getValueAtCurrentTime.bind(this)];
              this.keyframes = data.k;
              this.offsetTime = elem.data.st;
              this.k = true;
              this.kf = true;
              this._isFirstFrame = true;
              this.mult = mult || 1;
              this.elem = elem;
              this.container = container;
              this.comp = elem.comp;
              this.getValue = processEffectsSequence;
              this.setVValue = setVValue;
              this.interpolateValue = interpolateValue;
              this.frameId = -1;
              const arrLen = data.k[0].s.length;
              this.v = createTypedArray('float32', arrLen);
              this.pv = createTypedArray('float32', arrLen);
              for (i = 0; i < arrLen; i += 1) {
                this.v[i] = initFrame;
                this.pv[i] = initFrame;
              }
              this._caching = {
                lastFrame: initFrame,
                lastIndex: 0,
                value: createTypedArray('float32', arrLen),
              };
              this.addEffect = addEffect;
            }

            function getProp(elem, data, type, mult, container) {
              let p;
              if (!data.k.length) {
                p = new ValueProperty(elem, data, mult, container);
              } else if (typeof (data.k[0]) === 'number') {
                p = new MultiDimensionalProperty(elem, data, mult, container);
              } else {
                switch (type) {
                  case 0:
                    p = new KeyframedValueProperty(elem,data,mult, container);
                    break;
                  case 1:
                    p = new KeyframedMultidimensionalProperty(elem,data,mult, container);
                    break;
                }
              }
              if (p.effectsSequence.length) {
                container.addDynamicProperty(p);
              }
              return p;
            }

            const ob = {
              getProp: getProp,
            };
            return ob;
          }());
  const TransformPropertyFactory =
      (function() {
        function applyToMatrix(mat) {
          const _mdf = this._mdf;
          this.iterateDynamicProperties();
          this._mdf = this._mdf || _mdf;
          if (this.a) {
            mat.translate(-this.a.v[0], -this.a.v[1], this.a.v[2]);
          }
          if (this.s) {
            mat.scale(this.s.v[0], this.s.v[1], this.s.v[2]);
          }
          if (this.sk) {
            mat.skewFromAxis(-this.sk.v, this.sa.v);
          }
          if (this.r) {
            mat.rotate(-this.r.v);
          } else {
            mat.rotateZ(-this.rz.v).rotateY(this.ry.v).rotateX(this.rx.v).rotateZ(-this.or.v[2]).rotateY(this.or.v[1]).rotateX(this.or.v[0]);
          }
          if (this.data.p.s) {
            if (this.data.p.z) {
              mat.translate(this.px.v, this.py.v, -this.pz.v);
            } else {
              mat.translate(this.px.v, this.py.v, 0);
            }
          } else {
            mat.translate(this.p.v[0], this.p.v[1], -this.p.v[2]);
          }
        }
        function processKeys(forceRender) {
          if (this.elem.globalData.frameId === this.frameId) {
            return;
          }
          if (this._isDirty) {
            this.precalculateMatrix();
            this._isDirty = false;
          }

          this.iterateDynamicProperties();

          if (this._mdf || forceRender) {
            this.v.cloneFromProps(this.pre.props);
            if (this.appliedTransformations < 1) {
              this.v.translate(-this.a.v[0], -this.a.v[1], this.a.v[2]);
            }
            if(this.appliedTransformations < 2) {
              this.v.scale(this.s.v[0], this.s.v[1], this.s.v[2]);
            }
            if (this.sk && this.appliedTransformations < 3) {
              this.v.skewFromAxis(-this.sk.v, this.sa.v);
            }
            if (this.r && this.appliedTransformations < 4) {
              this.v.rotate(-this.r.v);
            } else if (!this.r && this.appliedTransformations < 4){
              this.v.rotateZ(-this.rz.v)
                  .rotateY(this.ry.v)
                  .rotateX(this.rx.v)
                  .rotateZ(-this.or.v[2])
                  .rotateY(this.or.v[1])
                  .rotateX(this.or.v[0]);
            }
            if (this.autoOriented) {
              let v1;
              let v2;
              const frameRate = this.elem.globalData.frameRate;
              if (this.p && this.p.keyframes && this.p.getValueAtTime) {
                if (this.p._caching.lastFrame + this.p.offsetTime <=
                    this.p.keyframes[0].t) {
                  v1 = this.p.getValueAtTime(
                      (this.p.keyframes[0].t + 0.01) / frameRate, 0);
                  v2 = this.p.getValueAtTime(
                      this.p.keyframes[0].t / frameRate, 0);
                } else if (
                    this.p._caching.lastFrame + this.p.offsetTime >=
                    this.p.keyframes[this.p.keyframes.length - 1].t) {
                  v1 = this.p.getValueAtTime(
                      (this.p.keyframes[this.p.keyframes.length - 1].t /
                       frameRate),
                      0);
                  v2 = this.p.getValueAtTime(
                      (this.p.keyframes[this.p.keyframes.length - 1].t - 0.01) /
                          frameRate,
                      0);
                } else {
                  v1 = this.p.pv;
                  v2 = this.p.getValueAtTime(
                      (this.p._caching.lastFrame + this.p.offsetTime - 0.01) /
                          frameRate,
                      this.p.offsetTime);
                }
              } else if (
                  this.px && this.px.keyframes && this.py.keyframes &&
                  this.px.getValueAtTime && this.py.getValueAtTime) {
                v1 = [];
                v2 = [];
                const px = this.px;
                const py = this.py;
                if (px._caching.lastFrame + px.offsetTime <=
                    px.keyframes[0].t) {
                  v1[0] = px.getValueAtTime(
                      (px.keyframes[0].t + 0.01) / frameRate, 0);
                  v1[1] = py.getValueAtTime(
                      (py.keyframes[0].t + 0.01) / frameRate, 0);
                  v2[0] = px.getValueAtTime((px.keyframes[0].t) / frameRate, 0);
                  v2[1] = py.getValueAtTime((py.keyframes[0].t) / frameRate, 0);
                } else if (
                    px._caching.lastFrame + px.offsetTime >=
                    px.keyframes[px.keyframes.length - 1].t) {
                  v1[0] = px.getValueAtTime(
                      (px.keyframes[px.keyframes.length - 1].t / frameRate), 0);
                  v1[1] = py.getValueAtTime(
                      (py.keyframes[py.keyframes.length - 1].t / frameRate), 0);
                  v2[0] = px.getValueAtTime(
                      (px.keyframes[px.keyframes.length - 1].t - 0.01) /
                          frameRate,
                      0);
                  v2[1] = py.getValueAtTime(
                      (py.keyframes[py.keyframes.length - 1].t - 0.01) /
                          frameRate,
                      0);
                } else {
                  v1 = [px.pv, py.pv];
                  v2[0] = px.getValueAtTime(
                      (px._caching.lastFrame + px.offsetTime - 0.01) /
                          frameRate,
                      px.offsetTime);
                  v2[1] = py.getValueAtTime(
                      (py._caching.lastFrame + py.offsetTime - 0.01) /
                          frameRate,
                      py.offsetTime);
                }
              }
              this.v.rotate(-Math.atan2(v1[1] - v2[1], v1[0] - v2[0]));
            }
            if(this.data.p && this.data.p.s){
              if (this.data.p.z) {
                this.v.translate(this.px.v, this.py.v, -this.pz.v);
              } else {
                this.v.translate(this.px.v, this.py.v, 0);
              }
            }else{
              this.v.translate(this.p.v[0], this.p.v[1], -this.p.v[2]);
            }
          }
          this.frameId = this.elem.globalData.frameId;
        }

        function precalculateMatrix() {
          if (!this.a.k) {
            this.pre.translate(-this.a.v[0], -this.a.v[1], this.a.v[2]);
            this.appliedTransformations = 1;
          } else {
            return;
          }
          if (!this.s.effectsSequence.length) {
            this.pre.scale(this.s.v[0], this.s.v[1], this.s.v[2]);
            this.appliedTransformations = 2;
          } else {
            return;
          }
          if (this.sk) {
            if(!this.sk.effectsSequence.length && !this.sa.effectsSequence.length) {
              this.pre.skewFromAxis(-this.sk.v, this.sa.v);
              this.appliedTransformations = 3;
            } else {
              return;
            }
          }
          if (this.r) {
            if(!this.r.effectsSequence.length) {
              this.pre.rotate(-this.r.v);
              this.appliedTransformations = 4;
            } else {
              return;
            }
          } else if (
              !this.rz.effectsSequence.length &&
              !this.ry.effectsSequence.length &&
              !this.rx.effectsSequence.length &&
              !this.or.effectsSequence.length) {
            this.pre.rotateZ(-this.rz.v).rotateY(this.ry.v).rotateX(this.rx.v).rotateZ(-this.or.v[2]).rotateY(this.or.v[1]).rotateX(this.or.v[0]);
            this.appliedTransformations = 4;
          }
        }

        function autoOrient() {
          //
          // let prevP = this.getValueAtTime();
        }

        function addDynamicProperty(prop) {
          this._addDynamicProperty(prop);
          this.elem.addDynamicProperty(prop);
          this._isDirty = true;
        }

        function TransformProperty(elem, data, container) {
          this.elem = elem;
          this.frameId = -1;
          this.propType = 'transform';
          this.data = data;
          this.v = new Matrix();
          // Precalculated matrix with non animated properties
          this.pre = new Matrix();
          this.appliedTransformations = 0;
          this.initDynamicPropertyContainer(container || elem);
          if (data.p && data.p.s) {
            this.px = PropertyFactory.getProp(elem,data.p.x,0,0,this);
            this.py = PropertyFactory.getProp(elem,data.p.y,0,0,this);
            if(data.p.z){
              this.pz = PropertyFactory.getProp(elem, data.p.z, 0, 0, this);
            }
          } else {
            this.p = PropertyFactory.getProp(elem,data.p || {k:[0,0,0]},1,0,this);
          }
          if (data.rx) {
            this.rx = PropertyFactory.getProp(elem, data.rx, 0, degToRads, this);
            this.ry = PropertyFactory.getProp(elem, data.ry, 0, degToRads, this);
            this.rz = PropertyFactory.getProp(elem, data.rz, 0, degToRads, this);
            if(data.or.k[0].ti) {
              let i;
              const len = data.or.k.length;
              for (i = 0; i < len; i += 1) {
                data.or.k[i].to = data.or.k[i].ti = null;
              }
            }
            this.or = PropertyFactory.getProp(elem, data.or, 1, degToRads, this);
            //sh Indicates it needs to be capped between -180 and 180
            this.or.sh = true;
          } else {
            this.r = PropertyFactory.getProp(elem, data.r || {k: 0}, 0, degToRads, this);
          }
          if (data.sk) {
            this.sk = PropertyFactory.getProp(elem, data.sk, 0, degToRads, this);
            this.sa = PropertyFactory.getProp(elem, data.sa, 0, degToRads, this);
          }
          this.a = PropertyFactory.getProp(
              elem, data.a || {k: [0, 0, 0]}, 1, 0, this);
          this.s = PropertyFactory.getProp(
              elem, data.s || {k: [100, 100, 100]}, 1, 0.01, this);
          // Opacity is not part of the transform properties, that's why it
          // won't use this.dynamicProperties. That way transforms won't get
          // updated if opacity changes.
          if (data.o) {
            this.o = PropertyFactory.getProp(elem,data.o,0,0.01,elem);
          } else {
            this.o = {_mdf:false,v:1};
          }
          this._isDirty = true;
          if (!this.dynamicProperties.length) {
            this.getValue(true);
          }
        }

        TransformProperty.prototype = {
          applyToMatrix: applyToMatrix,
          getValue: processKeys,
          precalculateMatrix: precalculateMatrix,
          autoOrient: autoOrient,
        };

        extendPrototype([DynamicPropertyContainer], TransformProperty);
        TransformProperty.prototype.addDynamicProperty = addDynamicProperty;
        TransformProperty.prototype._addDynamicProperty =
            DynamicPropertyContainer.prototype.addDynamicProperty;

        function getTransformProperty(elem, data, container) {
          return new TransformProperty(elem, data, container);
        }

        return {
          getTransformProperty: getTransformProperty,
        };
      }());
  function ShapePath() {
    this.c = false;
    this._length = 0;
    this._maxLength = 8;
    this.v = createSizedArray(this._maxLength);
    this.o = createSizedArray(this._maxLength);
    this.i = createSizedArray(this._maxLength);
  }

  ShapePath.prototype.setPathData = function(closed, len) {
    this.c = closed;
    this.setLength(len);
    let i = 0;
    while (i < len) {
      this.v[i] = point_pool.newElement();
      this.o[i] = point_pool.newElement();
      this.i[i] = point_pool.newElement();
      i += 1;
    }
  };

  ShapePath.prototype.setLength = function(len) {
    while (this._maxLength < len) {
      this.doubleArrayLength();
    }
    this._length = len;
  };

  ShapePath.prototype.doubleArrayLength = function() {
    this.v = this.v.concat(createSizedArray(this._maxLength));
    this.i = this.i.concat(createSizedArray(this._maxLength));
    this.o = this.o.concat(createSizedArray(this._maxLength));
    this._maxLength *= 2;
  };

  ShapePath.prototype.setXYAt = function(x, y, type, pos, replace) {
    let arr;
    this._length = Math.max(this._length, pos + 1);
    if (this._length >= this._maxLength) {
      this.doubleArrayLength();
    }
    switch (type) {
      case 'v':
        arr = this.v;
        break;
      case 'i':
        arr = this.i;
        break;
      case 'o':
        arr = this.o;
        break;
    }
    if (!arr[pos] || (arr[pos] && !replace)) {
      arr[pos] = point_pool.newElement();
    }
    arr[pos][0] = x;
    arr[pos][1] = y;
  };

  ShapePath.prototype.setTripleAt = function(
      vX, vY, oX, oY, iX, iY, pos, replace) {
    this.setXYAt(vX, vY, 'v', pos, replace);
    this.setXYAt(oX, oY, 'o', pos, replace);
    this.setXYAt(iX, iY, 'i', pos, replace);
  };

  ShapePath.prototype.reverse = function() {
    const newPath = new ShapePath();
    newPath.setPathData(this.c, this._length);
    const vertices = this.v;
    const outPoints = this.o;
    const inPoints = this.i;
    let init = 0;
    if (this.c) {
      newPath.setTripleAt(
          vertices[0][0], vertices[0][1], inPoints[0][0], inPoints[0][1],
          outPoints[0][0], outPoints[0][1], 0, false);
      init = 1;
    }
    let cnt = this._length - 1;
    const len = this._length;

    let i;
    for (i = init; i < len; i += 1) {
      newPath.setTripleAt(vertices[cnt][0], vertices[cnt][1], inPoints[cnt][0], inPoints[cnt][1], outPoints[cnt][0], outPoints[cnt][1], i, false);
      cnt -= 1;
    }
    return newPath;
  };
  const ShapePropertyFactory =
      (function() {
        const initFrame = -999999;

        function interpolateShape(frameNum, previousValue, caching) {
          let iterationIndex = caching.lastIndex;
          let keyPropS;
          let keyPropE;
          let isHold;
          let j;
          let k;
          let perc;
          let vertexValue;
          const kf = this.keyframes;
          if (frameNum < kf[0].t - this.offsetTime) {
            keyPropS = kf[0].s[0];
            isHold = true;
            iterationIndex = 0;
          } else if (frameNum >= kf[kf.length - 1].t - this.offsetTime) {
            keyPropS = kf[kf.length - 1].s ? kf[kf.length - 1].s[0] : kf[kf.length - 2].e[0];
            /*if(kf[kf.length - 1].s){
                keyPropS = kf[kf.length - 1].s[0];
            }else{
                keyPropS = kf[kf.length - 2].e[0];
            }*/
            isHold = true;
          } else {
            let i = iterationIndex;
            const len = kf.length - 1;
            let flag = true;
            let keyData;
            let nextKeyData;
            while(flag){
              keyData = kf[i];
              nextKeyData = kf[i + 1];
              if ((nextKeyData.t - this.offsetTime) > frameNum) {
                break;
              }
              if (i < len - 1) {
                i += 1;
              } else {
                flag = false;
              }
            }
            isHold = keyData.h === 1;
            iterationIndex = i;
            if(!isHold){
              if (frameNum >= nextKeyData.t - this.offsetTime) {
                perc = 1;
              } else if (frameNum < keyData.t - this.offsetTime) {
                perc = 0;
              } else {
                let fnc;
                if (keyData.__fnct) {
                  fnc = keyData.__fnct;
                } else {
                  fnc = BezierFactory
                            .getBezierEasing(
                                keyData.o.x, keyData.o.y, keyData.i.x,
                                keyData.i.y)
                            .get;
                  keyData.__fnct = fnc;
                }
                perc =
                    fnc((frameNum - (keyData.t - this.offsetTime)) /
                        ((nextKeyData.t - this.offsetTime) -
                         (keyData.t - this.offsetTime)));
              }
              keyPropE = nextKeyData.s ? nextKeyData.s[0] : keyData.e[0];
            }
            keyPropS = keyData.s[0];
          }
          const jLen = previousValue._length;
          const kLen = keyPropS.i[0].length;
          caching.lastIndex = iterationIndex;

          for (j = 0; j < jLen; j += 1) {
            for(k=0;k<kLen;k+=1){
              vertexValue = isHold ? keyPropS.i[j][k] :
                                     keyPropS.i[j][k] +
                      (keyPropE.i[j][k] - keyPropS.i[j][k]) * perc;
              previousValue.i[j][k] = vertexValue;
              vertexValue = isHold ? keyPropS.o[j][k] :
                                     keyPropS.o[j][k] +
                      (keyPropE.o[j][k] - keyPropS.o[j][k]) * perc;
              previousValue.o[j][k] = vertexValue;
              vertexValue = isHold ? keyPropS.v[j][k] :
                                     keyPropS.v[j][k] +
                      (keyPropE.v[j][k] - keyPropS.v[j][k]) * perc;
              previousValue.v[j][k] = vertexValue;
            }
          }
        }

        function interpolateShapeCurrentTime() {
          const frameNum = this.comp.renderedFrame - this.offsetTime;
          const initTime = this.keyframes[0].t - this.offsetTime;
          const endTime =
              this.keyframes[this.keyframes.length - 1].t - this.offsetTime;
          const lastFrame = this._caching.lastFrame;
          if (!(lastFrame !== initFrame &&
                ((lastFrame < initTime && frameNum < initTime) ||
                 (lastFrame > endTime && frameNum > endTime)))) {
            ////
            this._caching.lastIndex = lastFrame < frameNum ? this._caching.lastIndex : 0;
            this.interpolateShape(frameNum, this.pv, this._caching);
            ////
          }
          this._caching.lastFrame = frameNum;
          return this.pv;
        }

        function resetShape() {
          this.paths = this.localShapeCollection;
        }

        function shapesEqual(shape1, shape2) {
          if (shape1._length !== shape2._length || shape1.c !== shape2.c) {
            return false;
          }
          let i;
          const len = shape1._length;
          for (i = 0; i < len; i += 1) {
            if(shape1.v[i][0] !== shape2.v[i][0]
            || shape1.v[i][1] !== shape2.v[i][1]
            || shape1.o[i][0] !== shape2.o[i][0]
            || shape1.o[i][1] !== shape2.o[i][1]
            || shape1.i[i][0] !== shape2.i[i][0]
            || shape1.i[i][1] !== shape2.i[i][1]) {
              return false;
            }
          }
          return true;
        }

        function setVValue(newPath) {
          if (!shapesEqual(this.v, newPath)) {
            this.v = shape_pool.clone(newPath);
            this.localShapeCollection.releaseShapes();
            this.localShapeCollection.addShape(this.v);
            this._mdf = true;
            this.paths = this.localShapeCollection;
          }
        }

        function processEffectsSequence() {
          if (this.elem.globalData.frameId === this.frameId ||
              !this.effectsSequence.length) {
            return;
          }
          if (this.lock) {
            this.setVValue(this.pv);
            return;
          }
          this.lock = true;
          this._mdf = false;
          let finalValue = this.kf ? this.pv :
              this.data.ks         ? this.data.ks.k :
                                     this.data.pt.k;
          let i;
          const len = this.effectsSequence.length;
          for (i = 0; i < len; i += 1) {
            finalValue = this.effectsSequence[i](finalValue);
          }
          this.setVValue(finalValue);
          this.lock = false;
          this.frameId = this.elem.globalData.frameId;
        }

        function ShapeProperty(elem, data, type) {
          this.propType = 'shape';
          this.comp = elem.comp;
          this.container = elem;
          this.elem = elem;
          this.data = data;
          this.k = false;
          this.kf = false;
          this._mdf = false;
          const pathData = type === 3 ? data.pt.k : data.ks.k;
          this.v = shape_pool.clone(pathData);
          this.pv = shape_pool.clone(this.v);
          this.localShapeCollection = shapeCollection_pool.newShapeCollection();
          this.paths = this.localShapeCollection;
          this.paths.addShape(this.v);
          this.reset = resetShape;
          this.effectsSequence = [];
        }

        function addEffect(effectFunction) {
          this.effectsSequence.push(effectFunction);
          this.container.addDynamicProperty(this);
        }

        ShapeProperty.prototype.interpolateShape = interpolateShape;
        ShapeProperty.prototype.getValue = processEffectsSequence;
        ShapeProperty.prototype.setVValue = setVValue;
        ShapeProperty.prototype.addEffect = addEffect;

        function KeyframedShapeProperty(elem, data, type) {
          this.propType = 'shape';
          this.comp = elem.comp;
          this.elem = elem;
          this.container = elem;
          this.offsetTime = elem.data.st;
          this.keyframes = type === 3 ? data.pt.k : data.ks.k;
          this.k = true;
          this.kf = true;
          let i;
          const len = this.keyframes[0].s[0].i.length;
          const jLen = this.keyframes[0].s[0].i[0].length;
          this.v = shape_pool.newElement();
          this.v.setPathData(this.keyframes[0].s[0].c, len);
          this.pv = shape_pool.clone(this.v);
          this.localShapeCollection = shapeCollection_pool.newShapeCollection();
          this.paths = this.localShapeCollection;
          this.paths.addShape(this.v);
          this.lastFrame = initFrame;
          this.reset = resetShape;
          this._caching = {lastFrame: initFrame, lastIndex: 0};
          this.effectsSequence = [interpolateShapeCurrentTime.bind(this)];
        }
        KeyframedShapeProperty.prototype.getValue = processEffectsSequence;
        KeyframedShapeProperty.prototype.interpolateShape = interpolateShape;
        KeyframedShapeProperty.prototype.setVValue = setVValue;
        KeyframedShapeProperty.prototype.addEffect = addEffect;

        const EllShapeProperty = (function() {
          const cPoint = roundCorner;

          function EllShapeProperty(elem, data) {
            /*this.v = {
                v: createSizedArray(4),
                i: createSizedArray(4),
                o: createSizedArray(4),
                c: true
            };*/
            this.v = shape_pool.newElement();
            this.v.setPathData(true, 4);
            this.localShapeCollection = shapeCollection_pool.newShapeCollection();
            this.paths = this.localShapeCollection;
            this.localShapeCollection.addShape(this.v);
            this.d = data.d;
            this.elem = elem;
            this.comp = elem.comp;
            this.frameId = -1;
            this.initDynamicPropertyContainer(elem);
            this.p = PropertyFactory.getProp(elem,data.p,1,0,this);
            this.s = PropertyFactory.getProp(elem,data.s,1,0,this);
            if(this.dynamicProperties.length){
              this.k = true;
            }else{
              this.k = false;
              this.convertEllToPath();
            }
          }

          EllShapeProperty.prototype = {
            reset: resetShape,
            getValue: function() {
              if (this.elem.globalData.frameId === this.frameId) {
                return;
              }
              this.frameId = this.elem.globalData.frameId;
              this.iterateDynamicProperties();

              if (this._mdf) {
                this.convertEllToPath();
              }
            },
            convertEllToPath: function() {
              const p0 = this.p.v[0];
              const p1 = this.p.v[1];
              const s0 = this.s.v[0] / 2;
              const s1 = this.s.v[1] / 2;
              const _cw = this.d !== 3;
              const _v = this.v;
              _v.v[0][0] = p0;
              _v.v[0][1] = p1 - s1;
              _v.v[1][0] = _cw ? p0 + s0 : p0 - s0;
              _v.v[1][1] = p1;
              _v.v[2][0] = p0;
              _v.v[2][1] = p1 + s1;
              _v.v[3][0] = _cw ? p0 - s0 : p0 + s0;
              _v.v[3][1] = p1;
              _v.i[0][0] = _cw ? p0 - s0 * cPoint : p0 + s0 * cPoint;
              _v.i[0][1] = p1 - s1;
              _v.i[1][0] = _cw ? p0 + s0 : p0 - s0;
              _v.i[1][1] = p1 - s1 * cPoint;
              _v.i[2][0] = _cw ? p0 + s0 * cPoint : p0 - s0 * cPoint;
              _v.i[2][1] = p1 + s1;
              _v.i[3][0] = _cw ? p0 - s0 : p0 + s0;
              _v.i[3][1] = p1 + s1 * cPoint;
              _v.o[0][0] = _cw ? p0 + s0 * cPoint : p0 - s0 * cPoint;
              _v.o[0][1] = p1 - s1;
              _v.o[1][0] = _cw ? p0 + s0 : p0 - s0;
              _v.o[1][1] = p1 + s1 * cPoint;
              _v.o[2][0] = _cw ? p0 - s0 * cPoint : p0 + s0 * cPoint;
              _v.o[2][1] = p1 + s1;
              _v.o[3][0] = _cw ? p0 - s0 : p0 + s0;
              _v.o[3][1] = p1 - s1 * cPoint;
            },
          };

          extendPrototype([DynamicPropertyContainer], EllShapeProperty);

          return EllShapeProperty;
        }());

        const StarShapeProperty =
            (function() {
              function StarShapeProperty(elem, data) {
                this.v = shape_pool.newElement();
                this.v.setPathData(true, 0);
                this.elem = elem;
                this.comp = elem.comp;
                this.data = data;
                this.frameId = -1;
                this.d = data.d;
                this.initDynamicPropertyContainer(elem);
                if (data.sy === 1) {
                  this.ir = PropertyFactory.getProp(elem, data.ir, 0, 0, this);
                  this.is =
                      PropertyFactory.getProp(elem, data.is, 0, 0.01, this);
                  this.convertToPath = this.convertStarToPath;
                } else {
                  this.convertToPath = this.convertPolygonToPath;
                }
                this.pt = PropertyFactory.getProp(elem, data.pt, 0, 0, this);
                this.p = PropertyFactory.getProp(elem, data.p, 1, 0, this);
                this.r =
                    PropertyFactory.getProp(elem, data.r, 0, degToRads, this);
                this.or = PropertyFactory.getProp(elem, data.or, 0, 0, this);
                this.os = PropertyFactory.getProp(elem, data.os, 0, 0.01, this);
                this.localShapeCollection =
                    shapeCollection_pool.newShapeCollection();
                this.localShapeCollection.addShape(this.v);
                this.paths = this.localShapeCollection;
                if (this.dynamicProperties.length) {
                  this.k = true;
                } else {
                  this.k = false;
                  this.convertToPath();
                }
              }

              StarShapeProperty.prototype = {
                reset: resetShape,
                getValue: function() {
                  if (this.elem.globalData.frameId === this.frameId) {
                    return;
                  }
                  this.frameId = this.elem.globalData.frameId;
                  this.iterateDynamicProperties();
                  if (this._mdf) {
                    this.convertToPath();
                  }
                },
                convertStarToPath: function() {
                  const numPts = Math.floor(this.pt.v) * 2;
                  const angle = Math.PI * 2 / numPts;
                  /*this.v.v.length = numPts;
                  this.v.i.length = numPts;
                  this.v.o.length = numPts;*/
                  let longFlag = true;
                  const longRad = this.or.v;
                  const shortRad = this.ir.v;
                  const longRound = this.os.v;
                  const shortRound = this.is.v;
                  const longPerimSegment = 2 * Math.PI * longRad / (numPts * 2);
                  const shortPerimSegment =
                      2 * Math.PI * shortRad / (numPts * 2);
                  let i;
                  let rad;
                  let roundness;
                  let perimSegment;
                  let currentAng = -Math.PI / 2;
                  currentAng += this.r.v;
                  const dir = this.data.d === 3 ? -1 : 1;
                  this.v._length = 0;
                  for (i = 0; i < numPts; i += 1) {
                    rad = longFlag ? longRad : shortRad;
                    roundness = longFlag ? longRound : shortRound;
                    perimSegment = longFlag ? longPerimSegment : shortPerimSegment;
                    let x = rad * Math.cos(currentAng);
                    let y = rad * Math.sin(currentAng);
                    const ox =
                        x === 0 && y === 0 ? 0 : y / Math.sqrt(x * x + y * y);
                    const oy =
                        x === 0 && y === 0 ? 0 : -x / Math.sqrt(x * x + y * y);
                    x +=  + this.p.v[0];
                    y +=  + this.p.v[1];
                    this.v.setTripleAt(x,y,x-ox*perimSegment*roundness*dir,y-oy*perimSegment*roundness*dir,x+ox*perimSegment*roundness*dir,y+oy*perimSegment*roundness*dir, i, true);

                    /*this.v.v[i] = [x,y];
                    this.v.i[i] = [x+ox*perimSegment*roundness*dir,y+oy*perimSegment*roundness*dir];
                    this.v.o[i] = [x-ox*perimSegment*roundness*dir,y-oy*perimSegment*roundness*dir];
                    this.v._length = numPts;*/
                    longFlag = !longFlag;
                    currentAng += angle*dir;
                  }
                },
                convertPolygonToPath: function() {
                  const numPts = Math.floor(this.pt.v);
                  const angle = Math.PI * 2 / numPts;
                  const rad = this.or.v;
                  const roundness = this.os.v;
                  const perimSegment = 2 * Math.PI * rad / (numPts * 4);
                  let i;
                  let currentAng = -Math.PI / 2;
                  const dir = this.data.d === 3 ? -1 : 1;
                  currentAng += this.r.v;
                  this.v._length = 0;
                  for (i = 0; i < numPts; i += 1) {
                    let x = rad * Math.cos(currentAng);
                    let y = rad * Math.sin(currentAng);
                    const ox =
                        x === 0 && y === 0 ? 0 : y / Math.sqrt(x * x + y * y);
                    const oy =
                        x === 0 && y === 0 ? 0 : -x / Math.sqrt(x * x + y * y);
                    x +=  + this.p.v[0];
                    y +=  + this.p.v[1];
                    this.v.setTripleAt(x,y,x-ox*perimSegment*roundness*dir,y-oy*perimSegment*roundness*dir,x+ox*perimSegment*roundness*dir,y+oy*perimSegment*roundness*dir, i, true);
                    currentAng += angle*dir;
                  }
                  this.paths.length = 0;
                  this.paths[0] = this.v;
                },

              };
              extendPrototype([DynamicPropertyContainer], StarShapeProperty);

              return StarShapeProperty;
            }());

        const RectShapeProperty =
            (function() {
              function RectShapeProperty(elem, data) {
                this.v = shape_pool.newElement();
                this.v.c = true;
                this.localShapeCollection =
                    shapeCollection_pool.newShapeCollection();
                this.localShapeCollection.addShape(this.v);
                this.paths = this.localShapeCollection;
                this.elem = elem;
                this.comp = elem.comp;
                this.frameId = -1;
                this.d = data.d;
                this.initDynamicPropertyContainer(elem);
                this.p = PropertyFactory.getProp(elem, data.p, 1, 0, this);
                this.s = PropertyFactory.getProp(elem, data.s, 1, 0, this);
                this.r = PropertyFactory.getProp(elem, data.r, 0, 0, this);
                if (this.dynamicProperties.length) {
                  this.k = true;
                } else {
                  this.k = false;
                  this.convertRectToPath();
                }
              }

              RectShapeProperty.prototype = {
                convertRectToPath: function() {
                  const p0 = this.p.v[0];
                  const p1 = this.p.v[1];
                  const v0 = this.s.v[0] / 2;
                  const v1 = this.s.v[1] / 2;
                  const round = bm_min(v0, v1, this.r.v);
                  const cPoint = round * (1 - roundCorner);
                  this.v._length = 0;

                  if (this.d === 2 || this.d === 1) {
                    this.v.setTripleAt(p0+v0, p1-v1+round,p0+v0, p1-v1+round,p0+v0,p1-v1+cPoint,0, true);
                    this.v.setTripleAt(p0+v0, p1+v1-round,p0+v0, p1+v1-cPoint,p0+v0, p1+v1-round,1, true);
                    if(round!== 0){
                      this.v.setTripleAt(
                          p0 + v0 - round, p1 + v1, p0 + v0 - round, p1 + v1,
                          p0 + v0 - cPoint, p1 + v1, 2, true);
                      this.v.setTripleAt(
                          p0 - v0 + round, p1 + v1, p0 - v0 + cPoint, p1 + v1,
                          p0 - v0 + round, p1 + v1, 3, true);
                      this.v.setTripleAt(
                          p0 - v0, p1 + v1 - round, p0 - v0, p1 + v1 - round,
                          p0 - v0, p1 + v1 - cPoint, 4, true);
                      this.v.setTripleAt(
                          p0 - v0, p1 - v1 + round, p0 - v0, p1 - v1 + cPoint,
                          p0 - v0, p1 - v1 + round, 5, true);
                      this.v.setTripleAt(
                          p0 - v0 + round, p1 - v1, p0 - v0 + round, p1 - v1,
                          p0 - v0 + cPoint, p1 - v1, 6, true);
                      this.v.setTripleAt(
                          p0 + v0 - round, p1 - v1, p0 + v0 - cPoint, p1 - v1,
                          p0 + v0 - round, p1 - v1, 7, true);
                    } else {
                      this.v.setTripleAt(
                          p0 - v0, p1 + v1, p0 - v0 + cPoint, p1 + v1, p0 - v0,
                          p1 + v1, 2);
                      this.v.setTripleAt(
                          p0 - v0, p1 - v1, p0 - v0, p1 - v1 + cPoint, p0 - v0,
                          p1 - v1, 3);
                    }
                  } else {
                    this.v.setTripleAt(p0+v0,p1-v1+round,p0+v0,p1-v1+cPoint,p0+v0,p1-v1+round,0, true);
                    if(round!== 0){
                      this.v.setTripleAt(
                          p0 + v0 - round, p1 - v1, p0 + v0 - round, p1 - v1,
                          p0 + v0 - cPoint, p1 - v1, 1, true);
                      this.v.setTripleAt(
                          p0 - v0 + round, p1 - v1, p0 - v0 + cPoint, p1 - v1,
                          p0 - v0 + round, p1 - v1, 2, true);
                      this.v.setTripleAt(
                          p0 - v0, p1 - v1 + round, p0 - v0, p1 - v1 + round,
                          p0 - v0, p1 - v1 + cPoint, 3, true);
                      this.v.setTripleAt(
                          p0 - v0, p1 + v1 - round, p0 - v0, p1 + v1 - cPoint,
                          p0 - v0, p1 + v1 - round, 4, true);
                      this.v.setTripleAt(
                          p0 - v0 + round, p1 + v1, p0 - v0 + round, p1 + v1,
                          p0 - v0 + cPoint, p1 + v1, 5, true);
                      this.v.setTripleAt(
                          p0 + v0 - round, p1 + v1, p0 + v0 - cPoint, p1 + v1,
                          p0 + v0 - round, p1 + v1, 6, true);
                      this.v.setTripleAt(
                          p0 + v0, p1 + v1 - round, p0 + v0, p1 + v1 - round,
                          p0 + v0, p1 + v1 - cPoint, 7, true);
                    } else {
                      this.v.setTripleAt(
                          p0 - v0, p1 - v1, p0 - v0 + cPoint, p1 - v1, p0 - v0,
                          p1 - v1, 1, true);
                      this.v.setTripleAt(
                          p0 - v0, p1 + v1, p0 - v0, p1 + v1 - cPoint, p0 - v0,
                          p1 + v1, 2, true);
                      this.v.setTripleAt(
                          p0 + v0, p1 + v1, p0 + v0 - cPoint, p1 + v1, p0 + v0,
                          p1 + v1, 3, true);
                    }
                  }
                },
                getValue: function(frameNum) {
                  if (this.elem.globalData.frameId === this.frameId) {
                    return;
                  }
                  this.frameId = this.elem.globalData.frameId;
                  this.iterateDynamicProperties();
                  if (this._mdf) {
                    this.convertRectToPath();
                  }
                },
                reset: resetShape,
              };
              extendPrototype([DynamicPropertyContainer], RectShapeProperty);

              return RectShapeProperty;
            }());

        function getShapeProp(elem, data, type) {
          let prop;
          if (type === 3 || type === 4) {
            const dataProp = type === 3 ? data.pt : data.ks;
            const keys = dataProp.k;
            if(keys.length){
              prop = new KeyframedShapeProperty(elem, data, type);
            }else{
              prop = new ShapeProperty(elem, data, type);
            }
          } else if (type === 5) {
            prop = new RectShapeProperty(elem, data);
          } else if (type === 6) {
            prop = new EllShapeProperty(elem, data);
          } else if (type === 7) {
            prop = new StarShapeProperty(elem, data);
          }
          if (prop.k) {
            elem.addDynamicProperty(prop);
          }
          return prop;
        }

        function getConstructorFunction() {
          return ShapeProperty;
        }

        function getKeyframedConstructorFunction() {
          return KeyframedShapeProperty;
        }

        const ob = {};
        ob.getShapeProp = getShapeProp;
        ob.getConstructorFunction = getConstructorFunction;
        ob.getKeyframedConstructorFunction = getKeyframedConstructorFunction;
        return ob;
      }());
  const ShapeModifiers = (function() {
    const ob = {};
    const modifiers = {};
    ob.registerModifier = registerModifier;
    ob.getModifier = getModifier;

    function registerModifier(nm,factory){
      if (!modifiers[nm]) {
        modifiers[nm] = factory;
      }
    }

    function getModifier(nm,elem, data){
      return new modifiers[nm](elem, data);
    }

    return ob;
  }());

  function ShapeModifier() {}
  ShapeModifier.prototype.initModifierProperties = function() {};
  ShapeModifier.prototype.addShapeToModifier = function() {};
  ShapeModifier.prototype.addShape = function(data) {
    if(!this.closed){
      const shapeData = {
        shape: data.sh,
        data: data,
        localShapeCollection: shapeCollection_pool.newShapeCollection(),
      };
      this.shapes.push(shapeData);
      this.addShapeToModifier(shapeData);
      if (this._isAnimated) {
        data.setAsAnimated();
      }
    }
  };
  ShapeModifier.prototype.init = function(elem, data) {
    this.shapes = [];
    this.elem = elem;
    this.initDynamicPropertyContainer(elem);
    this.initModifierProperties(elem,data);
    this.frameId = initialDefaultFrame;
    this.closed = false;
    this.k = false;
    if(this.dynamicProperties.length){
      this.k = true;
    }else{
      this.getValue(true);
    }
  };
  ShapeModifier.prototype.processKeys = function() {
    if(this.elem.globalData.frameId === this.frameId){
      return;
    }
    this.frameId = this.elem.globalData.frameId;
    this.iterateDynamicProperties();
  };

  extendPrototype([DynamicPropertyContainer], ShapeModifier);
  function TrimModifier() {}
  extendPrototype([ShapeModifier], TrimModifier);
  TrimModifier.prototype.initModifierProperties = function(elem, data) {
    this.s = PropertyFactory.getProp(elem, data.s, 0, 0.01, this);
    this.e = PropertyFactory.getProp(elem, data.e, 0, 0.01, this);
    this.o = PropertyFactory.getProp(elem, data.o, 0, 0, this);
    this.sValue = 0;
    this.eValue = 0;
    this.getValue = this.processKeys;
    this.m = data.m;
    this._isAnimated = !!this.s.effectsSequence.length || !!this.e.effectsSequence.length || !!this.o.effectsSequence.length;
  };

  TrimModifier.prototype.addShapeToModifier = function(shapeData) {
    shapeData.pathsData = [];
  };

  TrimModifier.prototype.calculateShapeEdges = function(
      s, e, shapeLength, addedLength, totalModifierLength) {
    const segments = [];
    if (e <= 1) {
      segments.push({
        s: s,
        e: e,
      });
    } else if (s >= 1) {
      segments.push({
        s: s - 1,
        e: e - 1,
      });
    } else {
      segments.push({
        s: s,
        e: 1,
      });
      segments.push({
        s: 0,
        e: e - 1,
      });
    }
    const shapeSegments = [];
    let i;
    const len = segments.length;
    let segmentOb;
    for (i = 0; i < len; i += 1) {
      segmentOb = segments[i];
      if (segmentOb.e * totalModifierLength < addedLength ||
          segmentOb.s * totalModifierLength > addedLength + shapeLength) {
      } else {
        let shapeS;
        let shapeE;
        if (segmentOb.s * totalModifierLength <= addedLength) {
          shapeS = 0;
        } else {
          shapeS =
              (segmentOb.s * totalModifierLength - addedLength) / shapeLength;
        }
        if (segmentOb.e * totalModifierLength >= addedLength + shapeLength) {
          shapeE = 1;
        } else {
          shapeE =
              ((segmentOb.e * totalModifierLength - addedLength) / shapeLength);
        }
        shapeSegments.push([shapeS, shapeE]);
      }
    }
    if (!shapeSegments.length) {
      shapeSegments.push([0, 0]);
    }
    return shapeSegments;
  };

  TrimModifier.prototype.releasePathsData = function(pathsData) {
    let i;
    const len = pathsData.length;
    for (i = 0; i < len; i += 1) {
      segments_length_pool.release(pathsData[i]);
    }
    pathsData.length = 0;
    return pathsData;
  };

  TrimModifier.prototype.processShapes = function(_isFirstFrame) {
    let s;
    let e;
    if (this._mdf || _isFirstFrame) {
      let o = (this.o.v % 360) / 360;
      if (o < 0) {
        o += 1;
      }
      s = (this.s.v > 1 ? 1 : this.s.v < 0 ? 0 : this.s.v) + o;
      e = (this.e.v > 1 ? 1 : this.e.v < 0 ? 0 : this.e.v) + o;
      if (s === e) {
      }
      if (s > e) {
        const _s = s;
        s = e;
        e = _s;
      }
      s = Math.round(s * 10000) * 0.0001;
      e = Math.round(e * 10000) * 0.0001;
      this.sValue = s;
      this.eValue = e;
    } else {
      s = this.sValue;
      e = this.eValue;
    }
    let shapePaths;
    let i;
    const len = this.shapes.length;
    let j;
    let jLen;
    let pathsData;
    let pathData;
    let totalShapeLength;
    let totalModifierLength = 0;

    if (e === s) {
      for (i = 0; i < len; i += 1) {
        this.shapes[i].localShapeCollection.releaseShapes();
        this.shapes[i].shape._mdf = true;
        this.shapes[i].shape.paths = this.shapes[i].localShapeCollection;
      }
    } else if (!((e === 1 && s === 0) || (e===0 && s === 1))){
      const segments = [];
      let shapeData;
      let localShapeCollection;
      for (i = 0; i < len; i += 1) {
        shapeData = this.shapes[i];
        // if shape hasn't changed and trim properties haven't changed, cached
        // previous path can be used
        if (!shapeData.shape._mdf && !this._mdf && !_isFirstFrame &&
            this.m !== 2) {
          shapeData.shape.paths = shapeData.localShapeCollection;
        } else {
          shapePaths = shapeData.shape.paths;
          jLen = shapePaths._length;
          totalShapeLength = 0;
          if (!shapeData.shape._mdf && shapeData.pathsData.length) {
            totalShapeLength = shapeData.totalShapeLength;
          } else {
            pathsData = this.releasePathsData(shapeData.pathsData);
            for (j = 0; j < jLen; j += 1) {
              pathData = bez.getSegmentsLength(shapePaths.shapes[j]);
              pathsData.push(pathData);
              totalShapeLength += pathData.totalLength;
            }
            shapeData.totalShapeLength = totalShapeLength;
            shapeData.pathsData = pathsData;
          }

          totalModifierLength += totalShapeLength;
          shapeData.shape._mdf = true;
        }
      }
      let shapeS = s;
      let shapeE = e;
      let addedLength = 0;
      let edges;
      for (i = len - 1; i >= 0; i -= 1) {
        shapeData = this.shapes[i];
        if (shapeData.shape._mdf) {
          localShapeCollection = shapeData.localShapeCollection;
          localShapeCollection.releaseShapes();
          // if m === 2 means paths are trimmed individually so edges need to be
          // found for this specific shape relative to whoel group
          if (this.m === 2 && len > 1) {
            edges = this.calculateShapeEdges(
                s, e, shapeData.totalShapeLength, addedLength,
                totalModifierLength);
            addedLength += shapeData.totalShapeLength;
          } else {
            edges = [[shapeS, shapeE]];
          }
          jLen = edges.length;
          for (j = 0; j < jLen; j += 1) {
            shapeS = edges[j][0];
            shapeE = edges[j][1];
            segments.length = 0;
            if (shapeE <= 1) {
              segments.push({
                s: shapeData.totalShapeLength * shapeS,
                e: shapeData.totalShapeLength * shapeE,
              });
            } else if (shapeS >= 1) {
              segments.push({
                s: shapeData.totalShapeLength * (shapeS - 1),
                e: shapeData.totalShapeLength * (shapeE - 1),
              });
            } else {
              segments.push({
                s: shapeData.totalShapeLength * shapeS,
                e: shapeData.totalShapeLength,
              });
              segments.push({
                s: 0,
                e: shapeData.totalShapeLength * (shapeE - 1),
              });
            }
            let newShapesData = this.addShapes(shapeData, segments[0]);
            if (segments[0].s !== segments[0].e) {
              if (segments.length > 1) {
                const lastShapeInCollection =
                    shapeData.shape.paths
                        .shapes[shapeData.shape.paths._length - 1];
                if (lastShapeInCollection.c) {
                  const lastShape = newShapesData.pop();
                  this.addPaths(newShapesData, localShapeCollection);
                  newShapesData =
                      this.addShapes(shapeData, segments[1], lastShape);
                } else {
                  this.addPaths(newShapesData, localShapeCollection);
                  newShapesData = this.addShapes(shapeData, segments[1]);
                }
              }
              this.addPaths(newShapesData, localShapeCollection);
            }
          }
          shapeData.shape.paths = localShapeCollection;
        }
      }
    } else if (this._mdf) {
      for (i = 0; i < len; i += 1) {
        // Releasign Trim Cached paths data when no trim applied in case shapes
        // are modified inbetween. Don't remove this even if it's losing cached
        // info.
        this.shapes[i].pathsData.length = 0;
        this.shapes[i].shape._mdf = true;
      }
    }
  };

  TrimModifier.prototype.addPaths = function(newPaths, localShapeCollection) {
    let i;
    const len = newPaths.length;
    for (i = 0; i < len; i += 1) {
      localShapeCollection.addShape(newPaths[i]);
    }
  };

  TrimModifier.prototype.addSegment = function(
      pt1, pt2, pt3, pt4, shapePath, pos, newShape) {
    shapePath.setXYAt(pt2[0], pt2[1], 'o', pos);
    shapePath.setXYAt(pt3[0], pt3[1], 'i', pos + 1);
    if(newShape){
      shapePath.setXYAt(pt1[0], pt1[1], 'v', pos);
    }
    shapePath.setXYAt(pt4[0], pt4[1], 'v', pos + 1);
  };

  TrimModifier.prototype.addSegmentFromArray = function(
      points, shapePath, pos, newShape) {
    shapePath.setXYAt(points[1], points[5], 'o', pos);
    shapePath.setXYAt(points[2], points[6], 'i', pos + 1);
    if(newShape){
      shapePath.setXYAt(points[0], points[4], 'v', pos);
    }
    shapePath.setXYAt(points[3], points[7], 'v', pos + 1);
  };

  TrimModifier.prototype.addShapes = function(
      shapeData, shapeSegment, shapePath) {
    const pathsData = shapeData.pathsData;
    const shapePaths = shapeData.shape.paths.shapes;
    let i;
    const len = shapeData.shape.paths._length;
    let j;
    let jLen;
    let addedLength = 0;
    let currentLengthData;
    let segmentCount;
    let lengths;
    let segment;
    const shapes = [];
    let initPos;
    let newShape = true;
    if (!shapePath) {
      shapePath = shape_pool.newElement();
      segmentCount = 0;
      initPos = 0;
    } else {
      segmentCount = shapePath._length;
      initPos = shapePath._length;
    }
    shapes.push(shapePath);
    for (i = 0; i < len; i += 1) {
      lengths = pathsData[i].lengths;
      shapePath.c = shapePaths[i].c;
      jLen = shapePaths[i].c ? lengths.length : lengths.length + 1;
      for (j = 1; j < jLen; j += 1) {
        currentLengthData = lengths[j - 1];
        if (addedLength + currentLengthData.addedLength < shapeSegment.s) {
          addedLength += currentLengthData.addedLength;
          shapePath.c = false;
        } else if (addedLength > shapeSegment.e) {
          shapePath.c = false;
          break;
        } else {
          if (shapeSegment.s <= addedLength &&
              shapeSegment.e >= addedLength + currentLengthData.addedLength) {
            this.addSegment(
                shapePaths[i].v[j - 1], shapePaths[i].o[j - 1],
                shapePaths[i].i[j], shapePaths[i].v[j], shapePath, segmentCount,
                newShape);
            newShape = false;
          } else {
            segment = bez.getNewSegment(
                shapePaths[i].v[j - 1], shapePaths[i].v[j],
                shapePaths[i].o[j - 1], shapePaths[i].i[j],
                (shapeSegment.s - addedLength) / currentLengthData.addedLength,
                (shapeSegment.e - addedLength) / currentLengthData.addedLength,
                lengths[j - 1]);
            this.addSegmentFromArray(
                segment, shapePath, segmentCount, newShape);
            // this.addSegment(segment.pt1, segment.pt3, segment.pt4,
            // segment.pt2, shapePath, segmentCount, newShape);
            newShape = false;
            shapePath.c = false;
          }
          addedLength += currentLengthData.addedLength;
          segmentCount += 1;
        }
      }
      if (shapePaths[i].c && lengths.length) {
        currentLengthData = lengths[j - 1];
        if (addedLength <= shapeSegment.e) {
          const segmentLength = lengths[j - 1].addedLength;
          if (shapeSegment.s <= addedLength &&
              shapeSegment.e >= addedLength + segmentLength) {
            this.addSegment(
                shapePaths[i].v[j - 1], shapePaths[i].o[j - 1],
                shapePaths[i].i[0], shapePaths[i].v[0], shapePath, segmentCount,
                newShape);
            newShape = false;
          } else {
            segment = bez.getNewSegment(
                shapePaths[i].v[j - 1], shapePaths[i].v[0],
                shapePaths[i].o[j - 1], shapePaths[i].i[0],
                (shapeSegment.s - addedLength) / segmentLength,
                (shapeSegment.e - addedLength) / segmentLength, lengths[j - 1]);
            this.addSegmentFromArray(
                segment, shapePath, segmentCount, newShape);
            // this.addSegment(segment.pt1, segment.pt3, segment.pt4,
            // segment.pt2, shapePath, segmentCount, newShape);
            newShape = false;
            shapePath.c = false;
          }
        } else {
          shapePath.c = false;
        }
        addedLength += currentLengthData.addedLength;
        segmentCount += 1;
      }
      if (shapePath._length) {
        shapePath.setXYAt(
            shapePath.v[initPos][0], shapePath.v[initPos][1], 'i', initPos);
        shapePath.setXYAt(
            shapePath.v[shapePath._length - 1][0],
            shapePath.v[shapePath._length - 1][1], 'o', shapePath._length - 1);
      }
      if (addedLength > shapeSegment.e) {
        break;
      }
      if (i < len - 1) {
        shapePath = shape_pool.newElement();
        newShape = true;
        shapes.push(shapePath);
        segmentCount = 0;
      }
    }
    return shapes;
  };


  ShapeModifiers.registerModifier('tm', TrimModifier);
  function RoundCornersModifier() {}
  extendPrototype([ShapeModifier], RoundCornersModifier);
  RoundCornersModifier.prototype.initModifierProperties = function(elem, data) {
    this.getValue = this.processKeys;
    this.rd = PropertyFactory.getProp(elem,data.r,0,null,this);
    this._isAnimated = !!this.rd.effectsSequence.length;
  };

  RoundCornersModifier.prototype.processPath = function(path, round) {
    const cloned_path = shape_pool.newElement();
    cloned_path.c = path.c;
    let i;
    const len = path._length;
    let currentV;
    let currentI;
    let currentO;
    let closerV;
    let newV;
    let newO;
    let newI;
    let distance;
    let newPosPerc;
    let index = 0;
    let vX;
    let vY;
    let oX;
    let oY;
    let iX;
    let iY;
    for(i=0;i<len;i+=1){
      currentV = path.v[i];
      currentO = path.o[i];
      currentI = path.i[i];
      if (currentV[0] === currentO[0] && currentV[1] === currentO[1] &&
          currentV[0] === currentI[0] && currentV[1] === currentI[1]) {
        if ((i === 0 || i === len - 1) && !path.c) {
          cloned_path.setTripleAt(
              currentV[0], currentV[1], currentO[0], currentO[1], currentI[0],
              currentI[1], index);
          /*cloned_path.v[index] = currentV;
          cloned_path.o[index] = currentO;
          cloned_path.i[index] = currentI;*/
          index += 1;
        } else {
          if (i === 0) {
            closerV = path.v[len - 1];
          } else {
            closerV = path.v[i - 1];
          }
          distance = Math.sqrt(
              Math.pow(currentV[0] - closerV[0], 2) +
              Math.pow(currentV[1] - closerV[1], 2));
          newPosPerc = distance ? Math.min(distance / 2, round) / distance : 0;
          vX = iX = currentV[0] + (closerV[0] - currentV[0]) * newPosPerc;
          vY = iY = currentV[1] - (currentV[1] - closerV[1]) * newPosPerc;
          oX = vX - (vX - currentV[0]) * roundCorner;
          oY = vY - (vY - currentV[1]) * roundCorner;
          cloned_path.setTripleAt(vX, vY, oX, oY, iX, iY, index);
          index += 1;

          if (i === len - 1) {
            closerV = path.v[0];
          } else {
            closerV = path.v[i + 1];
          }
          distance = Math.sqrt(
              Math.pow(currentV[0] - closerV[0], 2) +
              Math.pow(currentV[1] - closerV[1], 2));
          newPosPerc = distance ? Math.min(distance / 2, round) / distance : 0;
          vX = oX = currentV[0] + (closerV[0] - currentV[0]) * newPosPerc;
          vY = oY = currentV[1] + (closerV[1] - currentV[1]) * newPosPerc;
          iX = vX - (vX - currentV[0]) * roundCorner;
          iY = vY - (vY - currentV[1]) * roundCorner;
          cloned_path.setTripleAt(vX, vY, oX, oY, iX, iY, index);
          index += 1;
        }
      } else {
        cloned_path.setTripleAt(
            path.v[i][0], path.v[i][1], path.o[i][0], path.o[i][1],
            path.i[i][0], path.i[i][1], index);
        index += 1;
      }
    }
    return cloned_path;
  };

  RoundCornersModifier.prototype.processShapes = function(_isFirstFrame) {
    let shapePaths;
    let i;
    const len = this.shapes.length;
    let j;
    let jLen;
    const rd = this.rd.v;

    if(rd !== 0){
      let shapeData;
      let newPaths;
      let localShapeCollection;
      for (i = 0; i < len; i += 1) {
        shapeData = this.shapes[i];
        newPaths = shapeData.shape.paths;
        localShapeCollection = shapeData.localShapeCollection;
        if (!(!shapeData.shape._mdf && !this._mdf && !_isFirstFrame)) {
          localShapeCollection.releaseShapes();
          shapeData.shape._mdf = true;
          shapePaths = shapeData.shape.paths.shapes;
          jLen = shapeData.shape.paths._length;
          for (j = 0; j < jLen; j += 1) {
            localShapeCollection.addShape(this.processPath(shapePaths[j], rd));
          }
        }
        shapeData.shape.paths = shapeData.localShapeCollection;
      }
    }
    if(!this.dynamicProperties.length){
      this._mdf = false;
    }
  };

  ShapeModifiers.registerModifier('rd', RoundCornersModifier);
  function RepeaterModifier() {}
  extendPrototype([ShapeModifier], RepeaterModifier);

  RepeaterModifier.prototype.initModifierProperties = function(elem, data) {
    this.getValue = this.processKeys;
    this.c = PropertyFactory.getProp(elem,data.c,0,null,this);
    this.o = PropertyFactory.getProp(elem,data.o,0,null,this);
    this.tr = TransformPropertyFactory.getTransformProperty(elem,data.tr,this);
    this.so = PropertyFactory.getProp(elem,data.tr.so,0,0.01,this);
    this.eo = PropertyFactory.getProp(elem,data.tr.eo,0,0.01,this);
    this.data = data;
    if(!this.dynamicProperties.length){
      this.getValue(true);
    }
    this._isAnimated = !!this.dynamicProperties.length;
    this.pMatrix = new Matrix();
    this.rMatrix = new Matrix();
    this.sMatrix = new Matrix();
    this.tMatrix = new Matrix();
    this.matrix = new Matrix();
  };

  RepeaterModifier.prototype.applyTransforms = function(
      pMatrix, rMatrix, sMatrix, transform, perc, inv) {
    const dir = inv ? -1 : 1;
    const scaleX = transform.s.v[0] + (1 - transform.s.v[0]) * (1 - perc);
    const scaleY = transform.s.v[1] + (1 - transform.s.v[1]) * (1 - perc);
    pMatrix.translate(transform.p.v[0] * dir * perc, transform.p.v[1] * dir * perc, transform.p.v[2]);
    rMatrix.translate(-transform.a.v[0], -transform.a.v[1], transform.a.v[2]);
    rMatrix.rotate(-transform.r.v * dir * perc);
    rMatrix.translate(transform.a.v[0], transform.a.v[1], transform.a.v[2]);
    sMatrix.translate(-transform.a.v[0], -transform.a.v[1], transform.a.v[2]);
    sMatrix.scale(inv ? 1/scaleX : scaleX, inv ? 1/scaleY : scaleY);
    sMatrix.translate(transform.a.v[0], transform.a.v[1], transform.a.v[2]);
  };

  RepeaterModifier.prototype.init = function(elem, arr, pos, elemsData) {
    this.elem = elem;
    this.arr = arr;
    this.pos = pos;
    this.elemsData = elemsData;
    this._currentCopies = 0;
    this._elements = [];
    this._groups = [];
    this.frameId = -1;
    this.initDynamicPropertyContainer(elem);
    this.initModifierProperties(elem,arr[pos]);
    let cont = 0;
    while(pos>0){
      pos -= 1;
      // this._elements.unshift(arr.splice(pos,1)[0]);
      this._elements.unshift(arr[pos]);
      cont += 1;
    }
    if(this.dynamicProperties.length){
      this.k = true;
    }else{
      this.getValue(true);
    }
  };

  RepeaterModifier.prototype.resetElements = function(elements) {
    let i;
    const len = elements.length;
    for(i = 0; i < len; i += 1) {
      elements[i]._processed = false;
      if (elements[i].ty === 'gr') {
        this.resetElements(elements[i].it);
      }
    }
  };

  RepeaterModifier.prototype.cloneElements = function(elements) {
    let i;
    const len = elements.length;
    // eslint-disable-next-line no-restricted-syntax
    const newElements = JSON.parse(JSON.stringify(elements));
    this.resetElements(newElements);
    return newElements;
  };

  RepeaterModifier.prototype.changeGroupRender = function(
      elements, renderFlag) {
    let i;
    const len = elements.length;
    for(i = 0; i < len; i += 1) {
      elements[i]._render = renderFlag;
      if (elements[i].ty === 'gr') {
        this.changeGroupRender(elements[i].it, renderFlag);
      }
    }
  };

  RepeaterModifier.prototype.processShapes = function(_isFirstFrame) {
    let items;
    let itemsTransform;
    let i;
    let dir;
    let cont;
    if(this._mdf || _isFirstFrame){
      const copies = Math.ceil(this.c.v);
      if (this._groups.length < copies) {
        while (this._groups.length < copies) {
          const group = {
            it: this.cloneElements(this._elements),
            ty: 'gr',
          };
          group.it.push({
            'a': {'a': 0, 'ix': 1, 'k': [0, 0]},
            'nm': 'Transform',
            'o': {'a': 0, 'ix': 7, 'k': 100},
            'p': {'a': 0, 'ix': 2, 'k': [0, 0]},
            'r': {
              'a': 1,
              'ix': 6,
              'k': [{s: 0, e: 0, t: 0}, {s: 0, e: 0, t: 1}],
            },
            's': {'a': 0, 'ix': 3, 'k': [100, 100]},
            'sa': {'a': 0, 'ix': 5, 'k': 0},
            'sk': {'a': 0, 'ix': 4, 'k': 0},
            'ty': 'tr',
          });

          this.arr.splice(0, 0, group);
          this._groups.splice(0, 0, group);
          this._currentCopies += 1;
        }
        this.elem.reloadShapes();
      }
      cont = 0;
      let renderFlag;
      for (i = 0; i <= this._groups.length - 1; i += 1) {
        renderFlag = cont < copies;
        this._groups[i]._render = renderFlag;
        this.changeGroupRender(this._groups[i].it, renderFlag);
        cont += 1;
      }

      this._currentCopies = copies;
      ////

      const offset = this.o.v;
      const offsetModulo = offset % 1;
      const roundOffset = offset > 0 ? Math.floor(offset) : Math.ceil(offset);
      let k;
      const tMat = this.tr.v.props;
      const pProps = this.pMatrix.props;
      const rProps = this.rMatrix.props;
      const sProps = this.sMatrix.props;
      this.pMatrix.reset();
      this.rMatrix.reset();
      this.sMatrix.reset();
      this.tMatrix.reset();
      this.matrix.reset();
      let iteration = 0;

      if (offset > 0) {
        while (iteration < roundOffset) {
          this.applyTransforms(
              this.pMatrix, this.rMatrix, this.sMatrix, this.tr, 1, false);
          iteration += 1;
        }
        if (offsetModulo) {
          this.applyTransforms(
              this.pMatrix, this.rMatrix, this.sMatrix, this.tr, offsetModulo,
              false);
          iteration += offsetModulo;
        }
      } else if (offset < 0) {
        while (iteration > roundOffset) {
          this.applyTransforms(
              this.pMatrix, this.rMatrix, this.sMatrix, this.tr, 1, true);
          iteration -= 1;
        }
        if (offsetModulo) {
          this.applyTransforms(
              this.pMatrix, this.rMatrix, this.sMatrix, this.tr, -offsetModulo,
              true);
          iteration -= offsetModulo;
        }
      }
      i = this.data.m === 1 ? 0 : this._currentCopies - 1;
      dir = this.data.m === 1 ? 1 : -1;
      cont = this._currentCopies;
      let j;
      let jLen;
      while (cont) {
        items = this.elemsData[i].it;
        itemsTransform = items[items.length - 1].transform.mProps.v.props;
        jLen = itemsTransform.length;
        items[items.length - 1].transform.mProps._mdf = true;
        items[items.length - 1].transform.op._mdf = true;
        items[items.length - 1].transform.op.v = this.so.v +
            (this.eo.v - this.so.v) * (i / (this._currentCopies - 1));
        if (iteration !== 0) {
          if ((i !== 0 && dir === 1) ||
              (i !== this._currentCopies - 1 && dir === -1)) {
            this.applyTransforms(
                this.pMatrix, this.rMatrix, this.sMatrix, this.tr, 1, false);
          }
          this.matrix.transform(
              rProps[0], rProps[1], rProps[2], rProps[3], rProps[4], rProps[5],
              rProps[6], rProps[7], rProps[8], rProps[9], rProps[10],
              rProps[11], rProps[12], rProps[13], rProps[14], rProps[15]);
          this.matrix.transform(
              sProps[0], sProps[1], sProps[2], sProps[3], sProps[4], sProps[5],
              sProps[6], sProps[7], sProps[8], sProps[9], sProps[10],
              sProps[11], sProps[12], sProps[13], sProps[14], sProps[15]);
          this.matrix.transform(
              pProps[0], pProps[1], pProps[2], pProps[3], pProps[4], pProps[5],
              pProps[6], pProps[7], pProps[8], pProps[9], pProps[10],
              pProps[11], pProps[12], pProps[13], pProps[14], pProps[15]);

          for (j = 0; j < jLen; j += 1) {
            itemsTransform[j] = this.matrix.props[j];
          }
          this.matrix.reset();
        } else {
          this.matrix.reset();
          for (j = 0; j < jLen; j += 1) {
            itemsTransform[j] = this.matrix.props[j];
          }
        }
        iteration += 1;
        cont -= 1;
        i += dir;
      }
    } else {
      cont = this._currentCopies;
      i = 0;
      dir = 1;
      while (cont) {
        items = this.elemsData[i].it;
        itemsTransform = items[items.length - 1].transform.mProps.v.props;
        items[items.length - 1].transform.mProps._mdf = false;
        items[items.length - 1].transform.op._mdf = false;
        cont -= 1;
        i += dir;
      }
    }
  };

  RepeaterModifier.prototype.addShape = function() {};

  ShapeModifiers.registerModifier('rp', RepeaterModifier);
  function ShapeCollection() {
    this._length = 0;
    this._maxLength = 4;
    this.shapes = createSizedArray(this._maxLength);
  }

  ShapeCollection.prototype.addShape = function(shapeData) {
    if (this._length === this._maxLength) {
      this.shapes = this.shapes.concat(createSizedArray(this._maxLength));
      this._maxLength *= 2;
    }
    this.shapes[this._length] = shapeData;
    this._length += 1;
  };

  ShapeCollection.prototype.releaseShapes = function() {
    let i;
    for (i = 0; i < this._length; i += 1) {
      shape_pool.release(this.shapes[i]);
    }
    this._length = 0;
  };
  function DashProperty(elem, data, renderer, container) {
    this.elem = elem;
    this.frameId = -1;
    this.dataProps = createSizedArray(data.length);
    this.renderer = renderer;
    this.k = false;
    this.dashStr = '';
    this.dashArray = createTypedArray('float32',  data.length ? data.length - 1 : 0);
    this.dashoffset = createTypedArray('float32',  1);
    this.initDynamicPropertyContainer(container);
    let i;
    const len = data.length || 0;
    let prop;
    for(i = 0; i < len; i += 1) {
      prop = PropertyFactory.getProp(elem, data[i].v, 0, 0, this);
      this.k = prop.k || this.k;
      this.dataProps[i] = {n: data[i].n, p: prop};
    }
    if(!this.k){
      this.getValue(true);
    }
    this._isAnimated = this.k;
  }

  DashProperty.prototype.getValue = function(forceRender) {
    if(this.elem.globalData.frameId === this.frameId && !forceRender){
      return;
    }
    this.frameId = this.elem.globalData.frameId;
    this.iterateDynamicProperties();
    this._mdf = this._mdf || forceRender;
    if (this._mdf) {
      let i = 0;
      const len = this.dataProps.length;
      if (this.renderer === 'svg') {
        this.dashStr = '';
      }
      for (i = 0; i < len; i += 1) {
        if (this.dataProps[i].n != 'o') {
          if (this.renderer === 'svg') {
            this.dashStr += ' ' + this.dataProps[i].p.v;
          } else {
            this.dashArray[i] = this.dataProps[i].p.v;
          }
        } else {
          this.dashoffset[0] = this.dataProps[i].p.v;
        }
      }
    }
  };
  extendPrototype([DynamicPropertyContainer], DashProperty);
  function GradientProperty(elem, data, container) {
    this.data = data;
    this.c = createTypedArray('uint8c', data.p*4);
    const cLength = data.k.k[0].s ? (data.k.k[0].s.length - data.p * 4) :
                                    data.k.k.length - data.p * 4;
    this.o = createTypedArray('float32', cLength);
    this._cmdf = false;
    this._omdf = false;
    this._collapsable = this.checkCollapsable();
    this._hasOpacity = cLength;
    this.initDynamicPropertyContainer(container);
    this.prop = PropertyFactory.getProp(elem,data.k,1,null,this);
    this.k = this.prop.k;
    this.getValue(true);
  }

  GradientProperty.prototype.comparePoints = function(values, points) {
    let i = 0;
    const len = this.o.length / 2;
    let diff;
    while(i < len) {
      diff = Math.abs(values[i * 4] - values[points * 4 + i * 2]);
      if (diff > 0.01) {
        return false;
      }
      i += 1;
    }
    return true;
  };

  GradientProperty.prototype.checkCollapsable = function() {
    if (this.o.length/2 !== this.c.length/4) {
      return false;
    }
    if (this.data.k.k[0].s) {
      let i = 0;
      const len = this.data.k.k.length;
      while (i < len) {
        if (!this.comparePoints(this.data.k.k[i].s, this.data.p)) {
          return false;
        }
        i += 1;
      }
    } else if(!this.comparePoints(this.data.k.k, this.data.p)) {
      return false;
    }
    return true;
  };

  GradientProperty.prototype.getValue = function(forceRender) {
    this.prop.getValue();
    this._mdf = false;
    this._cmdf = false;
    this._omdf = false;
    if(this.prop._mdf || forceRender){
      let i;
      let len = this.data.p * 4;
      let mult;
      let val;
      for (i = 0; i < len; i += 1) {
        mult = i % 4 === 0 ? 100 : 255;
        val = Math.round(this.prop.v[i] * mult);
        if (this.c[i] !== val) {
          this.c[i] = val;
          this._cmdf = !forceRender;
        }
      }
      if (this.o.length) {
        len = this.prop.v.length;
        for (i = this.data.p * 4; i < len; i += 1) {
          mult = i % 2 === 0 ? 100 : 1;
          val = i % 2 === 0 ? Math.round(this.prop.v[i] * 100) : this.prop.v[i];
          if (this.o[i - this.data.p * 4] !== val) {
            this.o[i - this.data.p * 4] = val;
            this._omdf = !forceRender;
          }
        }
      }
      this._mdf = !forceRender;
    }
  };

  extendPrototype([DynamicPropertyContainer], GradientProperty);
  const buildShapeString = function(pathNodes, length, closed, mat) {
    if (length === 0) {
      return '';
    }
    const _o = pathNodes.o;
    const _i = pathNodes.i;
    const _v = pathNodes.v;
    let i;
    let shapeString = ' M' + mat.applyToPointStringified(_v[0][0], _v[0][1]);
    for (i = 1; i < length; i += 1) {
      shapeString += ' C' +
          mat.applyToPointStringified(_o[i - 1][0], _o[i - 1][1]) + ' ' +
          mat.applyToPointStringified(_i[i][0], _i[i][1]) + ' ' +
          mat.applyToPointStringified(_v[i][0], _v[i][1]);
    }
    if (closed && length) {
      shapeString += ' C' +
          mat.applyToPointStringified(_o[i - 1][0], _o[i - 1][1]) + ' ' +
          mat.applyToPointStringified(_i[0][0], _i[0][1]) + ' ' +
          mat.applyToPointStringified(_v[0][0], _v[0][1]);
      shapeString += 'z';
    }
    return shapeString;
  };
  const ImagePreloader = function() {};

  const featureSupport = (function() {
    const ob = {
      maskType: true,
    };
    if (/MSIE 10/i.test(navigator.userAgent) ||
        /MSIE 9/i.test(navigator.userAgent) ||
        /rv:11.0/i.test(navigator.userAgent) ||
        /Edge\/\d./i.test(navigator.userAgent)) {
      ob.maskType = false;
    }
    return ob;
  }());
  const filtersFactory = (function() {
    const ob = {};
    ob.createFilter = createFilter;
    ob.createAlphaToLuminanceFilter = createAlphaToLuminanceFilter;

    function createFilter(filId) {
      const fil = createNS('filter');
      fil.setAttribute('id', filId);
      fil.setAttribute('filterUnits', 'objectBoundingBox');
      fil.setAttribute('x', '0%');
      fil.setAttribute('y', '0%');
      fil.setAttribute('width', '100%');
      fil.setAttribute('height', '100%');
      return fil;
    }

    function createAlphaToLuminanceFilter() {
      const feColorMatrix = createNS('feColorMatrix');
      feColorMatrix.setAttribute('type', 'matrix');
      feColorMatrix.setAttribute('color-interpolation-filters', 'sRGB');
      feColorMatrix.setAttribute(
          'values', '0 0 0 1 0  0 0 0 1 0  0 0 0 1 0  0 0 0 1 1');
      return feColorMatrix;
    }

    return ob;
  }());
  let assetLoader = (function() {
    function formatResponse(xhr) {
      if (xhr.response && typeof xhr.response === 'object') {
        return xhr.response;
      } else if (xhr.response && typeof xhr.response === 'string') {
        return JSON.parse(xhr.response);
      } else if (xhr.responseText) {
        return JSON.parse(xhr.responseText);
      }
    }

    function loadAsset(path, callback, errorCallback) {
      let response;
      const xhr = new XMLHttpRequest();
      xhr.open('GET', path, true);
      // set responseType after calling open or IE will break.
      try {
        // This crashes on Android WebView prior to KitKat
        xhr.responseType = 'json';
      } catch (err) {
      }
      xhr.send();
      xhr.onreadystatechange = function () {
          if (xhr.readyState == 4) {
              if(xhr.status == 200){
                response = formatResponse(xhr);
                callback(response);
              }else{
                  try{
                  response = formatResponse(xhr);
                  callback(response);
                  }catch(err){
                    if(errorCallback) {
                      errorCallback(err);
                    }
                  }
              }
          }
      };
    }
    return {
      load: loadAsset,
    };
  }());

  assetLoader = null;

  function TextAnimatorProperty(textData, renderType, elem) {
    this._isFirstFrame = true;
    this._hasMaskedPath = false;
    this._frameId = -1;
    this._textData = textData;
    this._renderType = renderType;
    this._elem = elem;
    this._animatorsData = createSizedArray(this._textData.a.length);
    this._pathData = {};
    this._moreOptions = {
      alignment: {},
    };
    this.renderedLetters = [];
    this.lettersChangedFlag = false;
    this.initDynamicPropertyContainer(elem);
  }

  TextAnimatorProperty.prototype.searchProperties = function() {
    let i;
    const len = this._textData.a.length;
    let animatorProps;
    const getProp = PropertyFactory.getProp;
    for(i=0;i<len;i+=1){
      animatorProps = this._textData.a[i];
      this._animatorsData[i] =
          new TextAnimatorDataProperty(this._elem, animatorProps, this);
    }
    if(this._textData.p && 'm' in this._textData.p){
      this._pathData = {
        f: getProp(this._elem, this._textData.p.f, 0, 0, this),
        l: getProp(this._elem, this._textData.p.l, 0, 0, this),
        r: this._textData.p.r,
        m: this._elem.maskManager.getMaskProperty(this._textData.p.m),
      };
      this._hasMaskedPath = true;
    } else {
      this._hasMaskedPath = false;
    }
    this._moreOptions.alignment = getProp(this._elem,this._textData.m.a,1,0,this);
  };

  TextAnimatorProperty.prototype.getMeasures = function(
      documentData, lettersChangedFlag) {
    this.lettersChangedFlag = lettersChangedFlag;
    if(!this._mdf && !this._isFirstFrame && !lettersChangedFlag && (!this._hasMaskedPath || !this._pathData.m._mdf)) {
      return;
    }
    this._isFirstFrame = false;
    const alignment = this._moreOptions.alignment.v;
    const animators = this._animatorsData;
    const textData = this._textData;
    const matrixHelper = this.mHelper;
    const renderType = this._renderType;
    let renderedLettersCount = this.renderedLetters.length;
    const data = this.data;
    let xPos;
    let yPos;
    let i;
    let len;
    const letters = documentData.l;
    let pathInfo;
    let currentLength;
    let currentPoint;
    let segmentLength;
    let flag;
    let pointInd;
    let segmentInd;
    let prevPoint;
    let points;
    let segments;
    let partialLength;
    let totalLength;
    let perc;
    let tanAngle;
    let mask;
    if(this._hasMaskedPath) {
      mask = this._pathData.m;
      if (!this._pathData.n || this._pathData._mdf) {
        let paths = mask.v;
        if (this._pathData.r) {
          paths = paths.reverse();
        }
        // TODO: release bezier data cached from previous pathInfo:
        // this._pathData.pi
        pathInfo = {
          tLength: 0,
          segments: [],
        };
        len = paths._length - 1;
        let bezierData;
        totalLength = 0;
        for (i = 0; i < len; i += 1) {
          bezierData = bez.buildBezierData(
              paths.v[i], paths.v[i + 1],
              [paths.o[i][0] - paths.v[i][0], paths.o[i][1] - paths.v[i][1]], [
                paths.i[i + 1][0] - paths.v[i + 1][0],
                paths.i[i + 1][1] - paths.v[i + 1][1],
              ]);
          pathInfo.tLength += bezierData.segmentLength;
          pathInfo.segments.push(bezierData);
          totalLength += bezierData.segmentLength;
        }
        i = len;
        if (mask.v.c) {
          bezierData = bez.buildBezierData(
              paths.v[i], paths.v[0],
              [paths.o[i][0] - paths.v[i][0], paths.o[i][1] - paths.v[i][1]],
              [paths.i[0][0] - paths.v[0][0], paths.i[0][1] - paths.v[0][1]]);
          pathInfo.tLength += bezierData.segmentLength;
          pathInfo.segments.push(bezierData);
          totalLength += bezierData.segmentLength;
        }
        this._pathData.pi = pathInfo;
      }
      pathInfo = this._pathData.pi;

      currentLength = this._pathData.f.v;
      segmentInd = 0;
      pointInd = 1;
      segmentLength = 0;
      flag = true;
      segments = pathInfo.segments;
      if (currentLength < 0 && mask.v.c) {
        if (pathInfo.tLength < Math.abs(currentLength)) {
          currentLength = -Math.abs(currentLength) % pathInfo.tLength;
        }
        segmentInd = segments.length - 1;
        points = segments[segmentInd].points;
        pointInd = points.length - 1;
        while (currentLength < 0) {
          currentLength += points[pointInd].partialLength;
          pointInd -= 1;
          if (pointInd < 0) {
            segmentInd -= 1;
            points = segments[segmentInd].points;
            pointInd = points.length - 1;
          }
        }
      }
      points = segments[segmentInd].points;
      prevPoint = points[pointInd - 1];
      currentPoint = points[pointInd];
      partialLength = currentPoint.partialLength;
    }


    len = letters.length;
    xPos = 0;
    yPos = 0;
    const yOff = documentData.finalSize * 1.2 * 0.714;
    let firstLine = true;
    let animatorProps;
    let animatorSelector;
    let j;
    let letterValue;

    const jLen = animators.length;
    let lastLetter;

    let mult;
    let ind = -1;
    let offf;
    let xPathPos;
    let yPathPos;
    const initPathPos = currentLength;
    const initSegmentInd = segmentInd;
    const initPointInd = pointInd;
    let currentLine = -1;
    let elemOpacity;
    let sc;
    let sw;
    let fc;
    let k;
    let lineLength = 0;
    let letterSw;
    let letterSc;
    let letterFc;
    let letterM = '';
    let letterP = this.defaultPropsArray;
    let letterO;

    //
    if(documentData.j === 2 || documentData.j === 1) {
      let animatorJustifyOffset = 0;
      let animatorFirstCharOffset = 0;
      const justifyOffsetMult = documentData.j === 2 ? -0.5 : -1;
      let lastIndex = 0;
      let isNewLine = true;

      for (i = 0; i < len; i += 1) {
        if (letters[i].n) {
          if (animatorJustifyOffset) {
            animatorJustifyOffset += animatorFirstCharOffset;
          }
          while (lastIndex < i) {
            letters[lastIndex].animatorJustifyOffset = animatorJustifyOffset;
            lastIndex += 1;
          }
          animatorJustifyOffset = 0;
          isNewLine = true;
        } else {
          for (j = 0; j < jLen; j += 1) {
            animatorProps = animators[j].a;
            if (animatorProps.t.propType) {
              if (isNewLine && documentData.j === 2) {
                animatorFirstCharOffset +=
                    animatorProps.t.v * justifyOffsetMult;
              }
              animatorSelector = animators[j].s;
              mult = animatorSelector.getMult(
                  letters[i].anIndexes[j], textData.a[j].s.totalChars);
              if (mult.length) {
                animatorJustifyOffset +=
                    animatorProps.t.v * mult[0] * justifyOffsetMult;
              } else {
                animatorJustifyOffset +=
                    animatorProps.t.v * mult * justifyOffsetMult;
              }
            }
          }
          isNewLine = false;
        }
      }
      if (animatorJustifyOffset) {
        animatorJustifyOffset += animatorFirstCharOffset;
      }
      while (lastIndex < i) {
        letters[lastIndex].animatorJustifyOffset = animatorJustifyOffset;
        lastIndex += 1;
      }
    }
    //

    for( i = 0; i < len; i += 1) {
      matrixHelper.reset();
      elemOpacity = 1;
      if (letters[i].n) {
        xPos = 0;
        yPos += documentData.yOffset;
        yPos += firstLine ? 1 : 0;
        currentLength = initPathPos;
        firstLine = false;
        lineLength = 0;
        if (this._hasMaskedPath) {
          segmentInd = initSegmentInd;
          pointInd = initPointInd;
          points = segments[segmentInd].points;
          prevPoint = points[pointInd - 1];
          currentPoint = points[pointInd];
          partialLength = currentPoint.partialLength;
          segmentLength = 0;
        }
        letterO = letterSw = letterFc = letterM = '';
        letterP = this.defaultPropsArray;
      } else {
        if (this._hasMaskedPath) {
          if (currentLine !== letters[i].line) {
            switch (documentData.j) {
              case 1:
                currentLength +=
                    totalLength - documentData.lineWidths[letters[i].line];
                break;
              case 2:
                currentLength +=
                    (totalLength - documentData.lineWidths[letters[i].line]) /
                    2;
                break;
            }
            currentLine = letters[i].line;
          }
          if (ind !== letters[i].ind) {
            if (letters[ind]) {
              currentLength += letters[ind].extra;
            }
            currentLength += letters[i].an / 2;
            ind = letters[i].ind;
          }
          currentLength += alignment[0] * letters[i].an / 200;
          let animatorOffset = 0;
          for (j = 0; j < jLen; j += 1) {
            animatorProps = animators[j].a;
            if (animatorProps.p.propType) {
              animatorSelector = animators[j].s;
              mult = animatorSelector.getMult(
                  letters[i].anIndexes[j], textData.a[j].s.totalChars);
              if (mult.length) {
                animatorOffset += animatorProps.p.v[0] * mult[0];
              } else {
                animatorOffset += animatorProps.p.v[0] * mult;
              }
            }
            if (animatorProps.a.propType) {
              animatorSelector = animators[j].s;
              mult = animatorSelector.getMult(
                  letters[i].anIndexes[j], textData.a[j].s.totalChars);
              if (mult.length) {
                animatorOffset += animatorProps.a.v[0] * mult[0];
              } else {
                animatorOffset += animatorProps.a.v[0] * mult;
              }
            }
          }
          flag = true;
          while (flag) {
            if (segmentLength + partialLength >=
                    currentLength + animatorOffset ||
                !points) {
              perc = (currentLength + animatorOffset - segmentLength) /
                  currentPoint.partialLength;
              xPathPos = prevPoint.point[0] +
                  (currentPoint.point[0] - prevPoint.point[0]) * perc;
              yPathPos = prevPoint.point[1] +
                  (currentPoint.point[1] - prevPoint.point[1]) * perc;
              matrixHelper.translate(
                  -alignment[0] * letters[i].an / 200,
                  -(alignment[1] * yOff / 100));
              flag = false;
            } else if (points) {
              segmentLength += currentPoint.partialLength;
              pointInd += 1;
              if (pointInd >= points.length) {
                pointInd = 0;
                segmentInd += 1;
                if (!segments[segmentInd]) {
                  if (mask.v.c) {
                    pointInd = 0;
                    segmentInd = 0;
                    points = segments[segmentInd].points;
                  } else {
                    segmentLength -= currentPoint.partialLength;
                    points = null;
                  }
                } else {
                  points = segments[segmentInd].points;
                }
              }
              if (points) {
                prevPoint = currentPoint;
                currentPoint = points[pointInd];
                partialLength = currentPoint.partialLength;
              }
            }
          }
          offf = letters[i].an / 2 - letters[i].add;
          matrixHelper.translate(-offf, 0, 0);
        } else {
          offf = letters[i].an / 2 - letters[i].add;
          matrixHelper.translate(-offf, 0, 0);

          // Grouping alignment
          matrixHelper.translate(
              -alignment[0] * letters[i].an / 200, -alignment[1] * yOff / 100,
              0);
        }

        lineLength += letters[i].l / 2;
        for (j = 0; j < jLen; j += 1) {
          animatorProps = animators[j].a;
          if (animatorProps.t.propType) {
            animatorSelector = animators[j].s;
            mult = animatorSelector.getMult(
                letters[i].anIndexes[j], textData.a[j].s.totalChars);
            // This condition is to prevent applying tracking to first character
            // in each line. Might be better to use a boolean "isNewLine"
            if (xPos !== 0 || documentData.j !== 0) {
              if (this._hasMaskedPath) {
                if (mult.length) {
                  currentLength += animatorProps.t.v * mult[0];
                } else {
                  currentLength += animatorProps.t.v * mult;
                }
              } else {
                if (mult.length) {
                  xPos += animatorProps.t.v * mult[0];
                } else {
                  xPos += animatorProps.t.v * mult;
                }
              }
            }
          }
        }
        lineLength += letters[i].l / 2;
        if (documentData.strokeWidthAnim) {
          sw = documentData.sw || 0;
        }
        if (documentData.strokeColorAnim) {
          if (documentData.sc) {
            sc = [documentData.sc[0], documentData.sc[1], documentData.sc[2]];
          } else {
            sc = [0, 0, 0];
          }
        }
        if (documentData.fillColorAnim && documentData.fc) {
          fc = [documentData.fc[0], documentData.fc[1], documentData.fc[2]];
        }
        for (j = 0; j < jLen; j += 1) {
          animatorProps = animators[j].a;
          if (animatorProps.a.propType) {
            animatorSelector = animators[j].s;
            mult = animatorSelector.getMult(
                letters[i].anIndexes[j], textData.a[j].s.totalChars);

            if (mult.length) {
              matrixHelper.translate(
                  -animatorProps.a.v[0] * mult[0],
                  -animatorProps.a.v[1] * mult[1],
                  animatorProps.a.v[2] * mult[2]);
            } else {
              matrixHelper.translate(
                  -animatorProps.a.v[0] * mult, -animatorProps.a.v[1] * mult,
                  animatorProps.a.v[2] * mult);
            }
          }
        }
        for (j = 0; j < jLen; j += 1) {
          animatorProps = animators[j].a;
          if (animatorProps.s.propType) {
            animatorSelector = animators[j].s;
            mult = animatorSelector.getMult(
                letters[i].anIndexes[j], textData.a[j].s.totalChars);
            if (mult.length) {
              matrixHelper.scale(
                  1 + ((animatorProps.s.v[0] - 1) * mult[0]),
                  1 + ((animatorProps.s.v[1] - 1) * mult[1]), 1);
            } else {
              matrixHelper.scale(
                  1 + ((animatorProps.s.v[0] - 1) * mult),
                  1 + ((animatorProps.s.v[1] - 1) * mult), 1);
            }
          }
        }
        for (j = 0; j < jLen; j += 1) {
          animatorProps = animators[j].a;
          animatorSelector = animators[j].s;
          mult = animatorSelector.getMult(
              letters[i].anIndexes[j], textData.a[j].s.totalChars);
          if (animatorProps.sk.propType) {
            if (mult.length) {
              matrixHelper.skewFromAxis(
                  -animatorProps.sk.v * mult[0], animatorProps.sa.v * mult[1]);
            } else {
              matrixHelper.skewFromAxis(
                  -animatorProps.sk.v * mult, animatorProps.sa.v * mult);
            }
          }
          if (animatorProps.r.propType) {
            if (mult.length) {
              matrixHelper.rotateZ(-animatorProps.r.v * mult[2]);
            } else {
              matrixHelper.rotateZ(-animatorProps.r.v * mult);
            }
          }
          if (animatorProps.ry.propType) {
            if (mult.length) {
              matrixHelper.rotateY(animatorProps.ry.v * mult[1]);
            } else {
              matrixHelper.rotateY(animatorProps.ry.v * mult);
            }
          }
          if (animatorProps.rx.propType) {
            if (mult.length) {
              matrixHelper.rotateX(animatorProps.rx.v * mult[0]);
            } else {
              matrixHelper.rotateX(animatorProps.rx.v * mult);
            }
          }
          if (animatorProps.o.propType) {
            if (mult.length) {
              elemOpacity +=
                  ((animatorProps.o.v) * mult[0] - elemOpacity) * mult[0];
            } else {
              elemOpacity += ((animatorProps.o.v) * mult - elemOpacity) * mult;
            }
          }
          if (documentData.strokeWidthAnim && animatorProps.sw.propType) {
            if (mult.length) {
              sw += animatorProps.sw.v * mult[0];
            } else {
              sw += animatorProps.sw.v * mult;
            }
          }
          if (documentData.strokeColorAnim && animatorProps.sc.propType) {
            for (k = 0; k < 3; k += 1) {
              if (mult.length) {
                sc[k] = sc[k] + (animatorProps.sc.v[k] - sc[k]) * mult[0];
              } else {
                sc[k] = sc[k] + (animatorProps.sc.v[k] - sc[k]) * mult;
              }
            }
          }
          if (documentData.fillColorAnim && documentData.fc) {
            if (animatorProps.fc.propType) {
              for (k = 0; k < 3; k += 1) {
                if (mult.length) {
                  fc[k] = fc[k] + (animatorProps.fc.v[k] - fc[k]) * mult[0];
                } else {
                  fc[k] = fc[k] + (animatorProps.fc.v[k] - fc[k]) * mult;
                }
              }
            }
            if (animatorProps.fh.propType) {
              if (mult.length) {
                fc = addHueToRGB(fc, animatorProps.fh.v * mult[0]);
              } else {
                fc = addHueToRGB(fc, animatorProps.fh.v * mult);
              }
            }
            if (animatorProps.fs.propType) {
              if (mult.length) {
                fc = addSaturationToRGB(fc, animatorProps.fs.v * mult[0]);
              } else {
                fc = addSaturationToRGB(fc, animatorProps.fs.v * mult);
              }
            }
            if (animatorProps.fb.propType) {
              if (mult.length) {
                fc = addBrightnessToRGB(fc, animatorProps.fb.v * mult[0]);
              } else {
                fc = addBrightnessToRGB(fc, animatorProps.fb.v * mult);
              }
            }
          }
        }

        for (j = 0; j < jLen; j += 1) {
          animatorProps = animators[j].a;

          if (animatorProps.p.propType) {
            animatorSelector = animators[j].s;
            mult = animatorSelector.getMult(
                letters[i].anIndexes[j], textData.a[j].s.totalChars);
            if (this._hasMaskedPath) {
              if (mult.length) {
                matrixHelper.translate(
                    0, animatorProps.p.v[1] * mult[0],
                    -animatorProps.p.v[2] * mult[1]);
              } else {
                matrixHelper.translate(
                    0, animatorProps.p.v[1] * mult,
                    -animatorProps.p.v[2] * mult);
              }
            } else {
              if (mult.length) {
                matrixHelper.translate(
                    animatorProps.p.v[0] * mult[0],
                    animatorProps.p.v[1] * mult[1],
                    -animatorProps.p.v[2] * mult[2]);
              } else {
                matrixHelper.translate(
                    animatorProps.p.v[0] * mult, animatorProps.p.v[1] * mult,
                    -animatorProps.p.v[2] * mult);
              }
            }
          }
        }
        if (documentData.strokeWidthAnim) {
          letterSw = sw < 0 ? 0 : sw;
        }
        if (documentData.strokeColorAnim) {
          letterSc = 'rgb(' + Math.round(sc[0] * 255) + ',' +
              Math.round(sc[1] * 255) + ',' + Math.round(sc[2] * 255) + ')';
        }
        if (documentData.fillColorAnim && documentData.fc) {
          letterFc = 'rgb(' + Math.round(fc[0] * 255) + ',' +
              Math.round(fc[1] * 255) + ',' + Math.round(fc[2] * 255) + ')';
        }

        if (this._hasMaskedPath) {
          matrixHelper.translate(0, -documentData.ls);

          matrixHelper.translate(0, alignment[1] * yOff / 100 + yPos, 0);
          if (textData.p.p) {
            tanAngle = (currentPoint.point[1] - prevPoint.point[1]) /
                (currentPoint.point[0] - prevPoint.point[0]);
            let rot = Math.atan(tanAngle) * 180 / Math.PI;
            if (currentPoint.point[0] < prevPoint.point[0]) {
              rot += 180;
            }
            matrixHelper.rotate(-rot * Math.PI / 180);
          }
          matrixHelper.translate(xPathPos, yPathPos, 0);
          currentLength -= alignment[0] * letters[i].an / 200;
          if (letters[i + 1] && ind !== letters[i + 1].ind) {
            currentLength += letters[i].an / 2;
            currentLength += documentData.tr / 1000 * documentData.finalSize;
          }
        } else {
          matrixHelper.translate(xPos, yPos, 0);

          if (documentData.ps) {
            // matrixHelper.translate(documentData.ps[0],documentData.ps[1],0);
            matrixHelper.translate(
                documentData.ps[0], documentData.ps[1] + documentData.ascent,
                0);
          }
          switch (documentData.j) {
            case 1:
              matrixHelper.translate(
                  letters[i].animatorJustifyOffset +
                      documentData.justifyOffset +
                      (documentData.boxWidth -
                       documentData.lineWidths[letters[i].line]),
                  0, 0);
              break;
            case 2:
              matrixHelper.translate(
                  letters[i].animatorJustifyOffset +
                      documentData.justifyOffset +
                      (documentData.boxWidth -
                       documentData.lineWidths[letters[i].line]) /
                          2,
                  0, 0);
              break;
          }
          matrixHelper.translate(0, -documentData.ls);
          matrixHelper.translate(offf, 0, 0);
          matrixHelper.translate(
              alignment[0] * letters[i].an / 200, alignment[1] * yOff / 100, 0);
          xPos +=
              letters[i].l + documentData.tr / 1000 * documentData.finalSize;
        }
        if (renderType === 'html') {
          letterM = matrixHelper.toCSS();
        } else if (renderType === 'svg') {
          letterM = matrixHelper.to2dCSS();
        } else {
          letterP = [
            matrixHelper.props[0],
            matrixHelper.props[1],
            matrixHelper.props[2],
            matrixHelper.props[3],
            matrixHelper.props[4],
            matrixHelper.props[5],
            matrixHelper.props[6],
            matrixHelper.props[7],
            matrixHelper.props[8],
            matrixHelper.props[9],
            matrixHelper.props[10],
            matrixHelper.props[11],
            matrixHelper.props[12],
            matrixHelper.props[13],
            matrixHelper.props[14],
            matrixHelper.props[15],
          ];
        }
        letterO = elemOpacity;
      }

      if (renderedLettersCount <= i) {
        letterValue = new LetterProps(
            letterO, letterSw, letterSc, letterFc, letterM, letterP);
        this.renderedLetters.push(letterValue);
        renderedLettersCount += 1;
        this.lettersChangedFlag = true;
      } else {
        letterValue = this.renderedLetters[i];
        this.lettersChangedFlag =
            letterValue.update(
                letterO, letterSw, letterSc, letterFc, letterM, letterP) ||
            this.lettersChangedFlag;
      }
    }
  };

  TextAnimatorProperty.prototype.getValue = function() {
    if (this._elem.globalData.frameId === this._frameId) {
      return;
    }
    this._frameId = this._elem.globalData.frameId;
    this.iterateDynamicProperties();
  };

  TextAnimatorProperty.prototype.mHelper = new Matrix();
  TextAnimatorProperty.prototype.defaultPropsArray = [];
  extendPrototype([DynamicPropertyContainer], TextAnimatorProperty);
  function TextAnimatorDataProperty(elem, animatorProps, container) {
    const defaultData = {propType: false};
    const getProp = PropertyFactory.getProp;
    const textAnimator_animatables = animatorProps.a;
    this.a = {
      r: textAnimator_animatables.r ?
          getProp(elem, textAnimator_animatables.r, 0, degToRads, container) :
          defaultData,
      rx: textAnimator_animatables.rx ?
          getProp(elem, textAnimator_animatables.rx, 0, degToRads, container) :
          defaultData,
      ry: textAnimator_animatables.ry ?
          getProp(elem, textAnimator_animatables.ry, 0, degToRads, container) :
          defaultData,
      sk: textAnimator_animatables.sk ?
          getProp(elem, textAnimator_animatables.sk, 0, degToRads, container) :
          defaultData,
      sa: textAnimator_animatables.sa ?
          getProp(elem, textAnimator_animatables.sa, 0, degToRads, container) :
          defaultData,
      s: textAnimator_animatables.s ?
          getProp(elem, textAnimator_animatables.s, 1, 0.01, container) :
          defaultData,
      a: textAnimator_animatables.a ?
          getProp(elem, textAnimator_animatables.a, 1, 0, container) :
          defaultData,
      o: textAnimator_animatables.o ?
          getProp(elem, textAnimator_animatables.o, 0, 0.01, container) :
          defaultData,
      p: textAnimator_animatables.p ?
          getProp(elem, textAnimator_animatables.p, 1, 0, container) :
          defaultData,
      sw: textAnimator_animatables.sw ?
          getProp(elem, textAnimator_animatables.sw, 0, 0, container) :
          defaultData,
      sc: textAnimator_animatables.sc ?
          getProp(elem, textAnimator_animatables.sc, 1, 0, container) :
          defaultData,
      fc: textAnimator_animatables.fc ?
          getProp(elem, textAnimator_animatables.fc, 1, 0, container) :
          defaultData,
      fh: textAnimator_animatables.fh ?
          getProp(elem, textAnimator_animatables.fh, 0, 0, container) :
          defaultData,
      fs: textAnimator_animatables.fs ?
          getProp(elem, textAnimator_animatables.fs, 0, 0.01, container) :
          defaultData,
      fb: textAnimator_animatables.fb ?
          getProp(elem, textAnimator_animatables.fb, 0, 0.01, container) :
          defaultData,
      t: textAnimator_animatables.t ?
          getProp(elem, textAnimator_animatables.t, 0, 0, container) :
          defaultData,
    };

    this.s =
        TextSelectorProp.getTextSelectorProp(elem, animatorProps.s, container);
    this.s.t = animatorProps.s.t;
  }
  function LetterProps(o, sw, sc, fc, m, p) {
    this.o = o;
    this.sw = sw;
    this.sc = sc;
    this.fc = fc;
    this.m = m;
    this.p = p;
    this._mdf = {
      o: true,
      sw: !!sw,
      sc: !!sc,
      fc: !!fc,
      m: true,
      p: true,
    };
  }

  LetterProps.prototype.update = function(o, sw, sc, fc, m, p) {
    this._mdf.o = false;
    this._mdf.sw = false;
    this._mdf.sc = false;
    this._mdf.fc = false;
    this._mdf.m = false;
    this._mdf.p = false;
    let updated = false;

    if (this.o !== o) {
      this.o = o;
      this._mdf.o = true;
      updated = true;
    }
    if (this.sw !== sw) {
      this.sw = sw;
      this._mdf.sw = true;
      updated = true;
    }
    if (this.sc !== sc) {
      this.sc = sc;
      this._mdf.sc = true;
      updated = true;
    }
    if (this.fc !== fc) {
      this.fc = fc;
      this._mdf.fc = true;
      updated = true;
    }
    if (this.m !== m) {
      this.m = m;
      this._mdf.m = true;
      updated = true;
    }
    if (p.length &&
        (this.p[0] !== p[0] || this.p[1] !== p[1] || this.p[4] !== p[4] ||
         this.p[5] !== p[5] || this.p[12] !== p[12] || this.p[13] !== p[13])) {
      this.p = p;
      this._mdf.p = true;
      updated = true;
    }
    return updated;
  };
  function TextProperty(elem, data) {
    this._frameId = initialDefaultFrame;
    this.pv = '';
    this.v = '';
    this.kf = false;
    this._isFirstFrame = true;
    this._mdf = false;
    this.data = data;
    this.elem = elem;
    this.comp = this.elem.comp;
    this.keysIndex = 0;
    this.canResize = false;
    this.minimumFontSize = 1;
    this.effectsSequence = [];
    this.currentData = {
      ascent: 0,
      boxWidth: this.defaultBoxWidth,
      f: '',
      fStyle: '',
      fWeight: '',
      fc: '',
      j: '',
      justifyOffset: '',
      l: [],
      lh: 0,
      lineWidths: [],
      ls: '',
      of: '',
      s: '',
      sc: '',
      sw: 0,
      t: 0,
      tr: 0,
      sz: 0,
      ps: null,
      fillColorAnim: false,
      strokeColorAnim: false,
      strokeWidthAnim: false,
      yOffset: 0,
      finalSize: 0,
      finalText: [],
      finalLineHeight: 0,
      __complete: false,

    };
    this.copyData(this.currentData, this.data.d.k[0].s);

    if(!this.searchProperty()) {
      this.completeTextData(this.currentData);
    }
  }

  TextProperty.prototype.defaultBoxWidth = [0, 0];

  TextProperty.prototype.copyData = function(obj, data) {
    for (const s in data) {
      if (data.hasOwnProperty(s)) {
        obj[s] = data[s];
      }
    }
    return obj;
  };

  TextProperty.prototype.setCurrentData = function(data) {
    if(!data.__complete) {
      this.completeTextData(data);
    }
    this.currentData = data;
    this.currentData.boxWidth = this.currentData.boxWidth || this.defaultBoxWidth;
    this._mdf = true;
  };

  TextProperty.prototype.searchProperty = function() {
    return this.searchKeyframes();
  };

  TextProperty.prototype.searchKeyframes = function() {
    this.kf = this.data.d.k.length > 1;
    if(this.kf) {
      this.addEffect(this.getKeyframeValue.bind(this));
    }
    return this.kf;
  };

  TextProperty.prototype.addEffect = function(effectFunction) {
    this.effectsSequence.push(effectFunction);
    this.elem.addDynamicProperty(this);
  };

  TextProperty.prototype.getValue = function(_finalValue) {
    if((this.elem.globalData.frameId === this.frameId || !this.effectsSequence.length) && !_finalValue) {
      return;
    }
    this.currentData.t = this.data.d.k[this.keysIndex].s.t;
    const currentValue = this.currentData;
    const currentIndex = this.keysIndex;
    if(this.lock) {
      this.setCurrentData(this.currentData);
      return;
    }
    this.lock = true;
    this._mdf = false;
    let multipliedValue;
    let i;
    const len = this.effectsSequence.length;
    let finalValue = _finalValue || this.data.d.k[this.keysIndex].s;
    for(i = 0; i < len; i += 1) {
      // Checking if index changed to prevent creating a new object every time
      // the expression updates.
      if (currentIndex !== this.keysIndex) {
        finalValue = this.effectsSequence[i](finalValue, finalValue.t);
      } else {
        finalValue = this.effectsSequence[i](this.currentData, finalValue.t);
      }
    }
    if(currentValue !== finalValue) {
      this.setCurrentData(finalValue);
    }
    this.pv = this.v = this.currentData;
    this.lock = false;
    this.frameId = this.elem.globalData.frameId;
  };

  TextProperty.prototype.getKeyframeValue = function() {
    const textKeys = this.data.d.k;
    let textDocumentData;
    const frameNum = this.elem.comp.renderedFrame;
    let i = 0;
    const len = textKeys.length;
    while(i <= len - 1) {
      textDocumentData = textKeys[i].s;
      if (i === len - 1 || textKeys[i + 1].t > frameNum) {
        break;
      }
      i += 1;
    }
    if(this.keysIndex !== i) {
      this.keysIndex = i;
    }
    return this.data.d.k[this.keysIndex].s;
  };

  TextProperty.prototype.buildFinalText = function(text) {
    const combinedCharacters = FontManager.getCombinedCharacterCodes();
    const charactersArray = [];
    let i = 0;
    const len = text.length;
    while (i < len) {
      if (combinedCharacters.indexOf(text.charCodeAt(i)) !== -1) {
        charactersArray[charactersArray.length - 1] += text.charAt(i);
      } else {
        charactersArray.push(text.charAt(i));
      }
      i += 1;
    }
    return charactersArray;
  };

  TextProperty.prototype.completeTextData = function(documentData) {
    documentData.__complete = true;
    const fontManager = this.elem.globalData.fontManager;
    const data = this.data;
    const letters = [];
    let i;
    let len;
    let newLineFlag;
    let index = 0;
    let val;
    const anchorGrouping = data.m.g;
    let currentSize = 0;
    let currentPos = 0;
    let currentLine = 0;
    const lineWidths = [];
    let lineWidth = 0;
    let maxLineWidth = 0;
    let j;
    const fontData = fontManager.getFontByName(documentData.f);
    let charData;
    let cLength = 0;
    const styles = fontData.fStyle ? fontData.fStyle.split(' ') : [];

    let fWeight = 'normal';
    let fStyle = 'normal';
    len = styles.length;
    let styleName;
    for(i=0;i<len;i+=1){
      styleName = styles[i].toLowerCase();
      switch (styleName) {
        case 'italic':
          fStyle = 'italic';
          break;
        case 'bold':
          fWeight = '700';
          break;
        case 'black':
          fWeight = '900';
          break;
        case 'medium':
          fWeight = '500';
          break;
        case 'regular':
        case 'normal':
          fWeight = '400';
          break;
        case 'light':
        case 'thin':
          fWeight = '200';
          break;
      }
    }
    documentData.fWeight = fontData.fWeight || fWeight;
    documentData.fStyle = fStyle;
    len = documentData.t.length;
    documentData.finalSize = documentData.s;
    documentData.finalText = this.buildFinalText(documentData.t);
    documentData.finalLineHeight = documentData.lh;
    let trackingOffset = documentData.tr / 1000 * documentData.finalSize;
    let charCode;
    if(documentData.sz){
      let flag = true;
      const boxWidth = documentData.sz[0];
      const boxHeight = documentData.sz[1];
      let currentHeight;
      let finalText;
      while (flag) {
        finalText = this.buildFinalText(documentData.t);
        currentHeight = 0;
        lineWidth = 0;
        len = finalText.length;
        trackingOffset = documentData.tr / 1000 * documentData.finalSize;
        let lastSpaceIndex = -1;
        for (i = 0; i < len; i += 1) {
          charCode = finalText[i].charCodeAt(0);
          newLineFlag = false;
          if (finalText[i] === ' ') {
            lastSpaceIndex = i;
          } else if (charCode === 13 || charCode === 3) {
            lineWidth = 0;
            newLineFlag = true;
            currentHeight +=
                documentData.finalLineHeight || documentData.finalSize * 1.2;
          }
          if (fontManager.chars) {
            charData = fontManager.getCharData(
                finalText[i], fontData.fStyle, fontData.fFamily);
            cLength =
                newLineFlag ? 0 : charData.w * documentData.finalSize / 100;
          } else {
            // tCanvasHelper.font = documentData.s + 'px '+ fontData.fFamily;
            cLength = fontManager.measureText(
                finalText[i], documentData.f, documentData.finalSize);
          }
          if (lineWidth + cLength > boxWidth && finalText[i] !== ' ') {
            if (lastSpaceIndex === -1) {
              len += 1;
            } else {
              i = lastSpaceIndex;
            }
            currentHeight +=
                documentData.finalLineHeight || documentData.finalSize * 1.2;
            finalText.splice(i, lastSpaceIndex === i ? 1 : 0, '\r');
            // finalText = finalText.substr(0,i) + "\r" + finalText.substr(i ===
            // lastSpaceIndex ? i + 1 : i);
            lastSpaceIndex = -1;
            lineWidth = 0;
          } else {
            lineWidth += cLength;
            lineWidth += trackingOffset;
          }
        }
        currentHeight += fontData.ascent * documentData.finalSize / 100;
        if (this.canResize && documentData.finalSize > this.minimumFontSize &&
            boxHeight < currentHeight) {
          documentData.finalSize -= 1;
          documentData.finalLineHeight =
              documentData.finalSize * documentData.lh / documentData.s;
        } else {
          documentData.finalText = finalText;
          len = documentData.finalText.length;
          flag = false;
        }
      }
    }
    lineWidth = - trackingOffset;
    cLength = 0;
    let uncollapsedSpaces = 0;
    let currentChar;
    for (i = 0;i < len ;i += 1) {
      newLineFlag = false;
      currentChar = documentData.finalText[i];
      charCode = currentChar.charCodeAt(0);
      if (currentChar === ' ') {
        val = '\u00A0';
      } else if (charCode === 13 || charCode === 3) {
        uncollapsedSpaces = 0;
        lineWidths.push(lineWidth);
        maxLineWidth = lineWidth > maxLineWidth ? lineWidth : maxLineWidth;
        lineWidth = -2 * trackingOffset;
        val = '';
        newLineFlag = true;
        currentLine += 1;
      } else {
        val = documentData.finalText[i];
      }
      if (fontManager.chars) {
        charData = fontManager.getCharData(
            currentChar, fontData.fStyle,
            fontManager.getFontByName(documentData.f).fFamily);
        cLength = newLineFlag ? 0 : charData.w * documentData.finalSize / 100;
      } else {
        // let charWidth = fontManager.measureText(val, documentData.f,
        // documentData.finalSize); tCanvasHelper.font = documentData.finalSize
        // + 'px '+ fontManager.getFontByName(documentData.f).fFamily;
        cLength = fontManager.measureText(
            val, documentData.f, documentData.finalSize);
      }

      //
      if (currentChar === ' ') {
        uncollapsedSpaces += cLength + trackingOffset;
      } else {
        lineWidth += cLength + trackingOffset + uncollapsedSpaces;
        uncollapsedSpaces = 0;
      }
      letters.push({
        l: cLength,
        an: cLength,
        add: currentSize,
        n: newLineFlag,
        anIndexes: [],
        val: val,
        line: currentLine,
        animatorJustifyOffset: 0,
      });
      if (anchorGrouping == 2) {
        currentSize += cLength;
        if (val === '' || val === '\u00A0' || i === len - 1) {
          if (val === '' || val === '\u00A0') {
            currentSize -= cLength;
          }
          while (currentPos <= i) {
            letters[currentPos].an = currentSize;
            letters[currentPos].ind = index;
            letters[currentPos].extra = cLength;
            currentPos += 1;
          }
          index += 1;
          currentSize = 0;
        }
      } else if (anchorGrouping == 3) {
        currentSize += cLength;
        if (val === '' || i === len - 1) {
          if (val === '') {
            currentSize -= cLength;
          }
          while (currentPos <= i) {
            letters[currentPos].an = currentSize;
            letters[currentPos].ind = index;
            letters[currentPos].extra = cLength;
            currentPos += 1;
          }
          currentSize = 0;
          index += 1;
        }
      } else {
        letters[index].ind = index;
        letters[index].extra = 0;
        index += 1;
      }
    }
    documentData.l = letters;
    maxLineWidth = lineWidth > maxLineWidth ? lineWidth : maxLineWidth;
    lineWidths.push(lineWidth);
    if(documentData.sz){
      documentData.boxWidth = documentData.sz[0];
      documentData.justifyOffset = 0;
    }else{
      documentData.boxWidth = maxLineWidth;
      switch (documentData.j) {
        case 1:
          documentData.justifyOffset = -documentData.boxWidth;
          break;
        case 2:
          documentData.justifyOffset = -documentData.boxWidth / 2;
          break;
        default:
          documentData.justifyOffset = 0;
      }
    }
    documentData.lineWidths = lineWidths;

    const animators = data.a;
    let animatorData;
    let letterData;
    const jLen = animators.length;
    let based;
    let ind;
    const indexes = [];
    for(j=0;j<jLen;j+=1){
      animatorData = animators[j];
      if (animatorData.a.sc) {
        documentData.strokeColorAnim = true;
      }
      if (animatorData.a.sw) {
        documentData.strokeWidthAnim = true;
      }
      if (animatorData.a.fc || animatorData.a.fh || animatorData.a.fs ||
          animatorData.a.fb) {
        documentData.fillColorAnim = true;
      }
      ind = 0;
      based = animatorData.s.b;
      for (i = 0; i < len; i += 1) {
        letterData = letters[i];
        letterData.anIndexes[j] = ind;
        if ((based == 1 && letterData.val !== '') ||
            (based == 2 && letterData.val !== '' &&
             letterData.val !== '\u00A0') ||
            (based == 3 &&
             (letterData.n || letterData.val == '\u00A0' || i == len - 1)) ||
            (based == 4 && (letterData.n || i == len - 1))) {
          if (animatorData.s.rn === 1) {
            indexes.push(ind);
          }
          ind += 1;
        }
      }
      data.a[j].s.totalChars = ind;
      let currentInd = -1;
      let newInd;
      if (animatorData.s.rn === 1) {
        for (i = 0; i < len; i += 1) {
          letterData = letters[i];
          if (currentInd != letterData.anIndexes[j]) {
            currentInd = letterData.anIndexes[j];
            newInd = indexes.splice(
                Math.floor(Math.random() * indexes.length), 1)[0];
          }
          letterData.anIndexes[j] = newInd;
        }
      }
    }
    documentData.yOffset = documentData.finalLineHeight || documentData.finalSize*1.2;
    documentData.ls = documentData.ls || 0;
    documentData.ascent = fontData.ascent*documentData.finalSize/100;
  };

  TextProperty.prototype.updateDocumentData = function(newData, index) {
    index = index === undefined ? this.keysIndex : index;
    let dData = this.copyData({}, this.data.d.k[index].s);
    dData = this.copyData(dData, newData);
    this.data.d.k[index].s = dData;
    this.recalculate(index);
    this.elem.addDynamicProperty(this);
  };

  TextProperty.prototype.recalculate = function(index) {
    const dData = this.data.d.k[index].s;
    dData.__complete = false;
    this.keysIndex = 0;
    this._isFirstFrame = true;
    this.getValue(dData);
  };

  TextProperty.prototype.canResizeFont = function(_canResize) {
    this.canResize = _canResize;
    this.recalculate(this.keysIndex);
    this.elem.addDynamicProperty(this);
  };

  TextProperty.prototype.setMinimumFontSize = function(_fontValue) {
    this.minimumFontSize = Math.floor(_fontValue) || 1;
    this.recalculate(this.keysIndex);
    this.elem.addDynamicProperty(this);
  };

  const TextSelectorProp = (function() {
    const max = Math.max;
    const min = Math.min;
    const floor = Math.floor;

    function TextSelectorProp(elem,data){
      this._currentTextLength = -1;
      this.k = false;
      this.data = data;
      this.elem = elem;
      this.comp = elem.comp;
      this.finalS = 0;
      this.finalE = 0;
      this.initDynamicPropertyContainer(elem);
      this.s = PropertyFactory.getProp(elem, data.s || {k: 0}, 0, 0, this);
      if ('e' in data) {
        this.e = PropertyFactory.getProp(elem, data.e, 0, 0, this);
      } else {
        this.e = {v: 100};
      }
      this.o = PropertyFactory.getProp(elem, data.o || {k: 0}, 0, 0, this);
      this.xe = PropertyFactory.getProp(elem, data.xe || {k: 0}, 0, 0, this);
      this.ne = PropertyFactory.getProp(elem, data.ne || {k: 0}, 0, 0, this);
      this.a = PropertyFactory.getProp(elem, data.a, 0, 0.01, this);
      if (!this.dynamicProperties.length) {
        this.getValue();
      }
    }

    TextSelectorProp.prototype = {
      getMult: function(ind) {
        if (this._currentTextLength !==
            this.elem.textProperty.currentData.l.length) {
          this.getValue();
        }
        // let easer = bez.getEasingCurve(this.ne.v/100,0,1-this.xe.v/100,1);
        const easer =
            BezierFactory
                .getBezierEasing(this.ne.v / 100, 0, 1 - this.xe.v / 100, 1)
                .get;
        let mult = 0;
        const s = this.finalS;
        const e = this.finalE;
        const type = this.data.sh;
        if (type == 2) {
          if (e === s) {
            mult = ind >= e ? 1 : 0;
          } else {
            mult = max(0, min(0.5 / (e - s) + (ind - s) / (e - s), 1));
          }
          mult = easer(mult);
        } else if (type == 3) {
          if (e === s) {
            mult = ind >= e ? 0 : 1;
          } else {
            mult = 1 - max(0, min(0.5 / (e - s) + (ind - s) / (e - s), 1));
          }

          mult = easer(mult);
        } else if (type == 4) {
          if (e === s) {
            mult = 0;
          } else {
            mult = max(0, min(0.5 / (e - s) + (ind - s) / (e - s), 1));
            if (mult < 0.5) {
              mult *= 2;
            } else {
              mult = 1 - 2 * (mult - 0.5);
            }
          }
          mult = easer(mult);
        } else if (type == 5) {
          if (e === s) {
            mult = 0;
          } else {
            const tot = e - s;
            /*ind += 0.5;
            mult = -4/(tot*tot)*(ind*ind)+(4/tot)*ind;*/
            ind = min(max(0, ind + 0.5 - s), e - s);
            const x = -tot / 2 + ind;
            const a = tot / 2;
            mult = Math.sqrt(1 - (x * x) / (a * a));
          }
          mult = easer(mult);
        } else if (type == 6) {
          if (e === s) {
            mult = 0;
          } else {
            ind = min(max(0, ind + 0.5 - s), e - s);
            mult =
                (1 + (Math.cos((Math.PI + Math.PI * 2 * (ind) / (e - s))))) / 2;
            /*
             ind = Math.min(Math.max(s,ind),e-1);
             mult = (1+(Math.cos((Math.PI+Math.PI*2*(ind-s)/(e-1-s)))))/2;
             mult = Math.max(mult,(1/(e-1-s))/(e-1-s));*/
          }
          mult = easer(mult);
        } else {
          if (ind >= floor(s)) {
            if (ind - s < 0) {
              mult = 1 - (s - ind);
            } else {
              mult = max(0, min(e - ind, 1));
            }
          }
          mult = easer(mult);
        }
        return mult * this.a.v;
      },
      getValue: function(newCharsFlag) {
        this.iterateDynamicProperties();
        this._mdf = newCharsFlag || this._mdf;
        this._currentTextLength =
            this.elem.textProperty.currentData.l.length || 0;
        if (newCharsFlag && this.data.r === 2) {
          this.e.v = this._currentTextLength;
        }
        const divisor = this.data.r === 2 ? 1 : 100 / this.data.totalChars;
        const o = this.o.v / divisor;
        let s = this.s.v / divisor + o;
        let e = (this.e.v / divisor) + o;
        if (s > e) {
          const _s = s;
          s = e;
          e = _s;
        }
        this.finalS = s;
        this.finalE = e;
      },
    };
    extendPrototype([DynamicPropertyContainer], TextSelectorProp);

    function getTextSelectorProp(elem, data,arr) {
      return new TextSelectorProp(elem, data, arr);
    }

    return {
      getTextSelectorProp: getTextSelectorProp,
    };
  }());


  const pool_factory = (function() {
    return function(initialLength, _create, _release, _clone) {
      let _length = 0;
      let _maxLength = initialLength;
      let pool = createSizedArray(_maxLength);

      const ob = {
        newElement: newElement,
        release: release,
      };

      function newElement() {
        let element;
        if (_length) {
          _length -= 1;
          element = pool[_length];
        } else {
          element = _create();
        }
        return element;
      }

      function release(element) {
        if (_length === _maxLength) {
          pool = pooling.double(pool);
          _maxLength = _maxLength * 2;
        }
        if (_release) {
          _release(element);
        }
        pool[_length] = element;
        _length += 1;
      }

      function clone() {
        const clonedElement = newElement();
        return _clone(clonedElement);
      }

      return ob;
    };
  }());

  const pooling = (function() {
    function double(arr) {
      return arr.concat(createSizedArray(arr.length));
    }

    return {
      double: double,
    };
  }());
  const point_pool = (function() {
    function create() {
      return createTypedArray('float32', 2);
    }
    return pool_factory(8, create);
  }());
  const shape_pool = (function() {
    function create() {
      return new ShapePath();
    }

    function release(shapePath) {
      const len = shapePath._length;
      let i;
      for (i = 0; i < len; i += 1) {
        point_pool.release(shapePath.v[i]);
        point_pool.release(shapePath.i[i]);
        point_pool.release(shapePath.o[i]);
        shapePath.v[i] = null;
        shapePath.i[i] = null;
        shapePath.o[i] = null;
      }
      shapePath._length = 0;
      shapePath.c = false;
    }

    function clone(shape) {
      const cloned = factory.newElement();
      let i;
      const len = shape._length === undefined ? shape.v.length : shape._length;
      cloned.setLength(len);
      cloned.c = shape.c;
      let pt;

      for (i = 0; i < len; i += 1) {
        cloned.setTripleAt(
            shape.v[i][0], shape.v[i][1], shape.o[i][0], shape.o[i][1],
            shape.i[i][0], shape.i[i][1], i);
      }
      return cloned;
    }

    const factory = pool_factory(4, create, release);
    factory.clone = clone;

    return factory;
  }());
  const shapeCollection_pool = (function() {
    const ob = {
      newShapeCollection: newShapeCollection,
      release: release,
    };

    let _length = 0;
    let _maxLength = 4;
    let pool = createSizedArray(_maxLength);

    function newShapeCollection() {
      let shapeCollection;
      if (_length) {
        _length -= 1;
        shapeCollection = pool[_length];
      } else {
        shapeCollection = new ShapeCollection();
      }
      return shapeCollection;
    }

    function release(shapeCollection) {
      let i;
      const len = shapeCollection._length;
      for (i = 0; i < len; i += 1) {
        shape_pool.release(shapeCollection.shapes[i]);
      }
      shapeCollection._length = 0;

      if (_length === _maxLength) {
        pool = pooling.double(pool);
        _maxLength = _maxLength * 2;
      }
      pool[_length] = shapeCollection;
      _length += 1;
    }

    return ob;
  }());
  const segments_length_pool = (function() {
    function create() {
      return {
        lengths: [],
        totalLength: 0,
      };
    }

    function release(element) {
      let i;
      const len = element.lengths.length;
      for (i = 0; i < len; i += 1) {
        bezier_length_pool.release(element.lengths[i]);
      }
      element.lengths.length = 0;
    }

    return pool_factory(8, create, release);
  }());
  const bezier_length_pool = (function() {
    function create() {
      return {
        addedLength: 0,
        percents: createTypedArray('float32', defaultCurveSegments),
        lengths: createTypedArray('float32', defaultCurveSegments),
      };
    }
    return pool_factory(8, create);
  }());
  function BaseRenderer() {}
  BaseRenderer.prototype.checkLayers = function(num) {
    let i;
    const len = this.layers.length;
    let data;
    this.completeLayers = true;
    for (i = len - 1; i >= 0; i--) {
      if (!this.elements[i]) {
        data = this.layers[i];
        if (data.ip - data.st <= (num - this.layers[i].st) &&
            data.op - data.st > (num - this.layers[i].st)) {
          this.buildItem(i);
        }
      }
      this.completeLayers = this.elements[i] ? this.completeLayers : false;
    }
    this.checkPendingElements();
  };

  BaseRenderer.prototype.createItem = function(layer) {
    switch(layer.ty){
      case 2:
        return this.createImage(layer);
      case 0:
        return this.createComp(layer);
      case 1:
        return this.createSolid(layer);
      case 3:
        return this.createNull(layer);
      case 4:
        return this.createShape(layer);
      case 5:
        return this.createText(layer);
      case 13:
        return this.createCamera(layer);
    }
    return this.createNull(layer);
  };

  BaseRenderer.prototype.createCamera = function() {
    throw new Error('You\'re using a 3d camera. Try the html renderer.');
  };

  BaseRenderer.prototype.buildAllItems = function() {
    let i;
    const len = this.layers.length;
    for(i=0;i<len;i+=1){
      this.buildItem(i);
    }
    this.checkPendingElements();
  };

  BaseRenderer.prototype.includeLayers = function(newLayers) {
    this.completeLayers = false;
    let i;
    const len = newLayers.length;
    let j;
    const jLen = this.layers.length;
    for(i=0;i<len;i+=1){
      j = 0;
      while (j < jLen) {
        if (this.layers[j].id == newLayers[i].id) {
          this.layers[j] = newLayers[i];
          break;
        }
        j += 1;
      }
    }
  };

  BaseRenderer.prototype.setProjectInterface = function(pInterface) {
    this.globalData.projectInterface = pInterface;
  };

  BaseRenderer.prototype.initItems = function() {
    if(!this.globalData.progressiveLoad){
      this.buildAllItems();
    }
  };
  BaseRenderer.prototype.buildElementParenting = function(
      element, parentName, hierarchy) {
    const elements = this.elements;
    const layers = this.layers;
    let i = 0;
    const len = layers.length;
    while (i < len) {
      if (layers[i].ind == parentName) {
        if (!elements[i] || elements[i] === true) {
          this.buildItem(i);
          this.addPendingElement(element);
        } else {
          hierarchy.push(elements[i]);
          elements[i].setAsParent();
          if (layers[i].parent !== undefined) {
            this.buildElementParenting(element, layers[i].parent, hierarchy);
          } else {
            element.setHierarchy(hierarchy);
          }
        }
      }
      i += 1;
    }
  };

  BaseRenderer.prototype.addPendingElement = function(element) {
    this.pendingElements.push(element);
  };

  BaseRenderer.prototype.searchExtraCompositions = function(assets) {
    let i;
    const len = assets.length;
    for(i=0;i<len;i+=1){
      if (assets[i].xt) {
        const comp = this.createComp(assets[i]);
        comp.initExpressions();
        this.globalData.projectInterface.registerComposition(comp);
      }
    }
  };

  BaseRenderer.prototype.setupGlobalData = function(animData, fontsContainer) {
    this.globalData.fontManager = new FontManager();
    this.globalData.fontManager.addChars(animData.chars);
    this.globalData.fontManager.addFonts(animData.fonts, fontsContainer);
    this.globalData.getAssetData = this.animationItem.getAssetData.bind(this.animationItem);
    this.globalData.getAssetsPath = this.animationItem.getAssetsPath.bind(this.animationItem);
    this.globalData.imageLoader = this.animationItem.imagePreloader;
    this.globalData.frameId = 0;
    this.globalData.frameRate = animData.fr;
    this.globalData.nm = animData.nm;
    this.globalData.compSize = {
      w: animData.w,
      h: animData.h,
    };
  };
  function SVGRenderer(animationItem, config) {
    this.animationItem = animationItem;
    this.layers = null;
    this.renderedFrame = -1;
    this.svgElement = createNS('svg');
    let ariaLabel = '';
    if (config && config.title) {
      const titleElement = createNS('title');
      const titleId = createElementID();
      titleElement.setAttribute('id', titleId);
      titleElement.textContent = config.title;
      this.svgElement.appendChild(titleElement);
      ariaLabel += titleId;
    }
    if (config && config.description) {
      const descElement = createNS('desc');
      const descId = createElementID();
      descElement.setAttribute('id', descId);
      descElement.textContent = config.description;
      this.svgElement.appendChild(descElement);
      ariaLabel += ' ' + descId;
    }
    if (ariaLabel) {
      this.svgElement.setAttribute('aria-labelledby', ariaLabel);
    }
    const defs = createNS('defs');
    this.svgElement.appendChild(defs);
    const maskElement = createNS('g');
    this.svgElement.appendChild(maskElement);
    this.layerElement = maskElement;
    this.renderConfig = {
      preserveAspectRatio:
          (config && config.preserveAspectRatio) || 'xMidYMid meet',
      imagePreserveAspectRatio:
          (config && config.imagePreserveAspectRatio) || 'xMidYMid slice',
      progressiveLoad: (config && config.progressiveLoad) || false,
      hideOnTransparent:
          (config && config.hideOnTransparent === false) ? false : true,
      viewBoxOnly: (config && config.viewBoxOnly) || false,
      viewBoxSize: (config && config.viewBoxSize) || false,
      className: (config && config.className) || '',
    };

    this.globalData = {
      _mdf: false,
      frameNum: -1,
      defs: defs,
      renderConfig: this.renderConfig,
    };
    this.elements = [];
    this.pendingElements = [];
    this.destroyed = false;
    this.rendererType = 'svg';
  }

  extendPrototype([BaseRenderer], SVGRenderer);

  SVGRenderer.prototype.createNull = function(data) {
    return new NullElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.createShape = function(data) {
    return new SVGShapeElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.createText = function(data) {
    return new SVGTextElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.createImage = function(data) {
    return new IImageElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.createComp = function(data) {
    return new SVGCompElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.createSolid = function(data) {
    return new ISolidElement(data,this.globalData,this);
  };

  SVGRenderer.prototype.configAnimation = function(animData) {
    this.svgElement.setAttribute('xmlns','http://www.w3.org/2000/svg');
    if(this.renderConfig.viewBoxSize) {
      this.svgElement.setAttribute('viewBox', this.renderConfig.viewBoxSize);
    } else {
      this.svgElement.setAttribute(
          'viewBox', '0 0 ' + animData.w + ' ' + animData.h);
    }

    if(!this.renderConfig.viewBoxOnly) {
      this.svgElement.setAttribute('width', animData.w);
      this.svgElement.setAttribute('height', animData.h);
      this.svgElement.style.width = '100%';
      this.svgElement.style.height = '100%';
      this.svgElement.style.transform = 'translate3d(0,0,0)';
    }
    if(this.renderConfig.className) {
      this.svgElement.setAttribute('class', this.renderConfig.className);
    }
    this.svgElement.setAttribute('preserveAspectRatio',this.renderConfig.preserveAspectRatio);
    //this.layerElement.style.transform = 'translate3d(0,0,0)';
    //this.layerElement.style.transformOrigin = this.layerElement.style.mozTransformOrigin = this.layerElement.style.webkitTransformOrigin = this.layerElement.style['-webkit-transform'] = "0px 0px 0px";
    this.animationItem.wrapper.appendChild(this.svgElement);
    //Mask animation
    const defs = this.globalData.defs;

    this.setupGlobalData(animData, defs);
    this.globalData.progressiveLoad = this.renderConfig.progressiveLoad;
    this.data = animData;

    const maskElement = createNS('clipPath');
    const rect = createNS('rect');
    rect.setAttribute('width',animData.w);
    rect.setAttribute('height',animData.h);
    rect.setAttribute('x',0);
    rect.setAttribute('y',0);
    const maskId = createElementID();
    maskElement.setAttribute('id', maskId);
    maskElement.appendChild(rect);
    this.layerElement.setAttribute(
        'clip-path', 'url(' + locationHref + '#' + maskId + ')');

    defs.appendChild(maskElement);
    this.layers = animData.layers;
    this.elements = createSizedArray(animData.layers.length);
  };


  SVGRenderer.prototype.destroy = function() {
    this.animationItem.wrapper.innerHTML = '';
    this.layerElement = null;
    this.globalData.defs = null;
    let i;
    const len = this.layers ? this.layers.length : 0;
    for (i = 0; i < len; i++) {
      if (this.elements[i]) {
        this.elements[i].destroy();
      }
    }
    this.elements.length = 0;
    this.destroyed = true;
    this.animationItem = null;
  };

  SVGRenderer.prototype.updateContainerSize = function() {};

  SVGRenderer.prototype.buildItem = function(pos) {
    const elements = this.elements;
    if(elements[pos] || this.layers[pos].ty == 99){
      return;
    }
    elements[pos] = true;
    const element = this.createItem(this.layers[pos]);

    elements[pos] = element;
    if(expressionsPlugin){
      if (this.layers[pos].ty === 0) {
        this.globalData.projectInterface.registerComposition(element);
      }
      element.initExpressions();
    }
    this.appendElementInPos(element,pos);
    if(this.layers[pos].tt){
      if (!this.elements[pos - 1] || this.elements[pos - 1] === true) {
        this.buildItem(pos - 1);
        this.addPendingElement(element);
      } else {
        element.setMatte(elements[pos - 1].layerId);
      }
    }
  };

  SVGRenderer.prototype.checkPendingElements = function() {
    while(this.pendingElements.length){
      const element = this.pendingElements.pop();
      element.checkParenting();
      if (element.data.tt) {
        let i = 0;
        const len = this.elements.length;
        while (i < len) {
          if (this.elements[i] === element) {
            element.setMatte(this.elements[i - 1].layerId);
            break;
          }
          i += 1;
        }
      }
    }
  };

  SVGRenderer.prototype.renderFrame = function(num) {
    if(this.renderedFrame === num || this.destroyed){
      return;
    }
    if(num === null){
      num = this.renderedFrame;
    }else{
      this.renderedFrame = num;
    }
    // console.log('-------');
    // console.log('FRAME ',num);
    this.globalData.frameNum = num;
    this.globalData.frameId += 1;
    this.globalData.projectInterface.currentFrame = num;
    this.globalData._mdf = false;
    let i;
    const len = this.layers.length;
    if(!this.completeLayers){
      this.checkLayers(num);
    }
    for (i = len - 1; i >= 0; i--) {
      if (this.completeLayers || this.elements[i]) {
        this.elements[i].prepareFrame(num - this.layers[i].st);
      }
    }
    if(this.globalData._mdf) {
      for (i = 0; i < len; i += 1) {
        if (this.completeLayers || this.elements[i]) {
          this.elements[i].renderFrame();
        }
      }
    }
  };

  SVGRenderer.prototype.appendElementInPos = function(element, pos) {
    const newElement = element.getBaseElement();
    if(!newElement){
      return;
    }
    let i = 0;
    let nextElement;
    while(i<pos){
      if (this.elements[i] && this.elements[i] !== true &&
          this.elements[i].getBaseElement()) {
        nextElement = this.elements[i].getBaseElement();
      }
      i += 1;
    }
    if(nextElement){
      this.layerElement.insertBefore(newElement, nextElement);
    } else {
      this.layerElement.appendChild(newElement);
    }
  };

  SVGRenderer.prototype.hide = function() {
    this.layerElement.style.display = 'none';
  };

  SVGRenderer.prototype.show = function() {
    this.layerElement.style.display = 'block';
  };

  function CanvasRenderer(animationItem, config) {
    this.animationItem = animationItem;
    this.renderConfig = {
      clearCanvas: (config && config.clearCanvas !== undefined) ?
          config.clearCanvas :
          true,
      context: (config && config.context) || null,
      progressiveLoad: (config && config.progressiveLoad) || false,
      preserveAspectRatio:
          (config && config.preserveAspectRatio) || 'xMidYMid meet',
      imagePreserveAspectRatio:
          (config && config.imagePreserveAspectRatio) || 'xMidYMid slice',
      className: (config && config.className) || '',
    };
    this.renderConfig.dpr = (config && config.dpr) || 1;
    if (this.animationItem.wrapper) {
      this.renderConfig.dpr =
          (config && config.dpr) || window.devicePixelRatio || 1;
    }
    this.renderedFrame = -1;
    this.globalData = {
      frameNum: -1,
      _mdf: false,
      renderConfig: this.renderConfig,
      currentGlobalAlpha: -1,
    };
    this.contextData = new CVContextData();
    this.elements = [];
    this.pendingElements = [];
    this.transformMat = new Matrix();
    this.completeLayers = false;
    this.rendererType = 'canvas';
  }
  extendPrototype([BaseRenderer], CanvasRenderer);

  CanvasRenderer.prototype.createShape = function(data) {
    return new CVShapeElement(data, this.globalData, this);
  };

  CanvasRenderer.prototype.createText = function(data) {
    return new CVTextElement(data, this.globalData, this);
  };

  CanvasRenderer.prototype.createImage = function(data) {
    return new CVImageElement(data, this.globalData, this);
  };

  CanvasRenderer.prototype.createComp = function(data) {
    return new CVCompElement(data, this.globalData, this);
  };

  CanvasRenderer.prototype.createSolid = function(data) {
    return new CVSolidElement(data, this.globalData, this);
  };

  CanvasRenderer.prototype.createNull = SVGRenderer.prototype.createNull;

  CanvasRenderer.prototype.ctxTransform = function(props) {
    if(props[0] === 1 && props[1] === 0 && props[4] === 0 && props[5] === 1 && props[12] === 0 && props[13] === 0){
      return;
    }
    if(!this.renderConfig.clearCanvas){
      this.canvasContext.transform(
          props[0], props[1], props[4], props[5], props[12], props[13]);
      return;
    }
    this.transformMat.cloneFromProps(props);
    const cProps = this.contextData.cTr.props;
    this.transformMat.transform(cProps[0],cProps[1],cProps[2],cProps[3],cProps[4],cProps[5],cProps[6],cProps[7],cProps[8],cProps[9],cProps[10],cProps[11],cProps[12],cProps[13],cProps[14],cProps[15]);
    //this.contextData.cTr.transform(props[0],props[1],props[2],props[3],props[4],props[5],props[6],props[7],props[8],props[9],props[10],props[11],props[12],props[13],props[14],props[15]);
    this.contextData.cTr.cloneFromProps(this.transformMat.props);
    const trProps = this.contextData.cTr.props;
    this.canvasContext.setTransform(trProps[0],trProps[1],trProps[4],trProps[5],trProps[12],trProps[13]);
  };

  CanvasRenderer.prototype.ctxOpacity = function(op) {
    /*if(op === 1){
        return;
    }*/
    if(!this.renderConfig.clearCanvas){
      this.canvasContext.globalAlpha *= op < 0 ? 0 : op;
      this.globalData.currentGlobalAlpha = this.contextData.cO;
      return;
    }
    this.contextData.cO *= op < 0 ? 0 : op;
    if(this.globalData.currentGlobalAlpha !== this.contextData.cO) {
      this.canvasContext.globalAlpha = this.contextData.cO;
      this.globalData.currentGlobalAlpha = this.contextData.cO;
    }
  };

  CanvasRenderer.prototype.reset = function() {
    if(!this.renderConfig.clearCanvas){
      this.canvasContext.restore();
      return;
    }
    this.contextData.reset();
  };

  CanvasRenderer.prototype.save = function(actionFlag) {
    if(!this.renderConfig.clearCanvas){
      this.canvasContext.save();
      return;
    }
    if(actionFlag){
      this.canvasContext.save();
    }
    const props = this.contextData.cTr.props;
    if(this.contextData._length <= this.contextData.cArrPos) {
      this.contextData.duplicate();
    }
    let i;
    const arr = this.contextData.saved[this.contextData.cArrPos];
    for (i = 0; i < 16; i += 1) {
      arr[i] = props[i];
    }
    this.contextData.savedOp[this.contextData.cArrPos] = this.contextData.cO;
    this.contextData.cArrPos += 1;
  };

  CanvasRenderer.prototype.restore = function(actionFlag) {
    if(!this.renderConfig.clearCanvas){
      this.canvasContext.restore();
      return;
    }
    if(actionFlag){
      this.canvasContext.restore();
      this.globalData.blendMode = 'source-over';
    }
    this.contextData.cArrPos -= 1;
    let popped = this.contextData.saved[this.contextData.cArrPos];
    let i;
    const arr = this.contextData.cTr.props;
    for(i=0;i<16;i+=1){
      arr[i] = popped[i];
    }
    this.canvasContext.setTransform(popped[0],popped[1],popped[4],popped[5],popped[12],popped[13]);
    popped = this.contextData.savedOp[this.contextData.cArrPos];
    this.contextData.cO = popped;
    if(this.globalData.currentGlobalAlpha !== popped) {
      this.canvasContext.globalAlpha = popped;
      this.globalData.currentGlobalAlpha = popped;
    }
  };

  CanvasRenderer.prototype.configAnimation = function(animData) {
    if(this.animationItem.wrapper){
      this.animationItem.container = createTag('canvas');
      this.animationItem.container.style.width = '100%';
      this.animationItem.container.style.height = '100%';
      // this.animationItem.container.style.transform = 'translate3d(0,0,0)';
      // this.animationItem.container.style.webkitTransform =
      // 'translate3d(0,0,0)';
      this.animationItem.container.style.transformOrigin =
          this.animationItem.container.style.mozTransformOrigin =
              this.animationItem.container.style.webkitTransformOrigin =
                  this.animationItem.container.style['-webkit-transform'] =
                      '0px 0px 0px';
      this.animationItem.wrapper.appendChild(this.animationItem.container);
      this.canvasContext = this.animationItem.container.getContext('2d');
      if (this.renderConfig.className) {
        this.animationItem.container.setAttribute(
            'class', this.renderConfig.className);
      }
    }else{
      this.canvasContext = this.renderConfig.context;
    }
    this.data = animData;
    this.layers = animData.layers;
    this.transformCanvas = {
      w: animData.w,
      h: animData.h,
      sx: 0,
      sy: 0,
      tx: 0,
      ty: 0,
    };
    this.setupGlobalData(animData, document.body);
    this.globalData.canvasContext = this.canvasContext;
    this.globalData.renderer = this;
    this.globalData.isDashed = false;
    this.globalData.progressiveLoad = this.renderConfig.progressiveLoad;
    this.globalData.transformCanvas = this.transformCanvas;
    this.elements = createSizedArray(animData.layers.length);

    this.updateContainerSize();
  };

  CanvasRenderer.prototype.updateContainerSize = function() {
    this.reset();
    let elementWidth;
    let elementHeight;
    if(this.animationItem.wrapper && this.animationItem.container){
      elementWidth = this.animationItem.wrapper.offsetWidth;
      elementHeight = this.animationItem.wrapper.offsetHeight;
      this.animationItem.container.setAttribute(
          'width', elementWidth * this.renderConfig.dpr);
      this.animationItem.container.setAttribute(
          'height', elementHeight * this.renderConfig.dpr);
    }else{
      elementWidth = this.canvasContext.canvas.width * this.renderConfig.dpr;
      elementHeight = this.canvasContext.canvas.height * this.renderConfig.dpr;
    }
    let elementRel;
    let animationRel;
    if(this.renderConfig.preserveAspectRatio.indexOf('meet') !== -1 || this.renderConfig.preserveAspectRatio.indexOf('slice') !== -1){
      const par = this.renderConfig.preserveAspectRatio.split(' ');
      const fillType = par[1] || 'meet';
      const pos = par[0] || 'xMidYMid';
      const xPos = pos.substr(0, 4);
      const yPos = pos.substr(4);
      elementRel = elementWidth / elementHeight;
      animationRel = this.transformCanvas.w / this.transformCanvas.h;
      if (animationRel > elementRel && fillType === 'meet' ||
          animationRel < elementRel && fillType === 'slice') {
        this.transformCanvas.sx =
            elementWidth / (this.transformCanvas.w / this.renderConfig.dpr);
        this.transformCanvas.sy =
            elementWidth / (this.transformCanvas.w / this.renderConfig.dpr);
      } else {
        this.transformCanvas.sx =
            elementHeight / (this.transformCanvas.h / this.renderConfig.dpr);
        this.transformCanvas.sy =
            elementHeight / (this.transformCanvas.h / this.renderConfig.dpr);
      }

      if (xPos === 'xMid' &&
          ((animationRel < elementRel && fillType === 'meet') ||
           (animationRel > elementRel && fillType === 'slice'))) {
        this.transformCanvas.tx =
            (elementWidth -
             this.transformCanvas.w *
                 (elementHeight / this.transformCanvas.h)) /
            2 * this.renderConfig.dpr;
      } else if (
          xPos === 'xMax' &&
          ((animationRel < elementRel && fillType === 'meet') ||
           (animationRel > elementRel && fillType === 'slice'))) {
        this.transformCanvas.tx =
            (elementWidth -
             this.transformCanvas.w *
                 (elementHeight / this.transformCanvas.h)) *
            this.renderConfig.dpr;
      } else {
        this.transformCanvas.tx = 0;
      }
      if (yPos === 'YMid' &&
          ((animationRel > elementRel && fillType === 'meet') ||
           (animationRel < elementRel && fillType === 'slice'))) {
        this.transformCanvas.ty =
            ((elementHeight -
              this.transformCanvas.h *
                  (elementWidth / this.transformCanvas.w)) /
             2) *
            this.renderConfig.dpr;
      } else if (
          yPos === 'YMax' &&
          ((animationRel > elementRel && fillType === 'meet') ||
           (animationRel < elementRel && fillType === 'slice'))) {
        this.transformCanvas.ty =
            ((elementHeight -
              this.transformCanvas.h *
                  (elementWidth / this.transformCanvas.w))) *
            this.renderConfig.dpr;
      } else {
        this.transformCanvas.ty = 0;
      }

    } else if (this.renderConfig.preserveAspectRatio == 'none') {
      this.transformCanvas.sx =
          elementWidth / (this.transformCanvas.w / this.renderConfig.dpr);
      this.transformCanvas.sy =
          elementHeight / (this.transformCanvas.h / this.renderConfig.dpr);
      this.transformCanvas.tx = 0;
      this.transformCanvas.ty = 0;
    } else {
      this.transformCanvas.sx = this.renderConfig.dpr;
      this.transformCanvas.sy = this.renderConfig.dpr;
      this.transformCanvas.tx = 0;
      this.transformCanvas.ty = 0;
    }
    this.transformCanvas.props = [this.transformCanvas.sx,0,0,0,0,this.transformCanvas.sy,0,0,0,0,1,0,this.transformCanvas.tx,this.transformCanvas.ty,0,1];
    /*let i, len = this.elements.length;
    for(i=0;i<len;i+=1){
        if(this.elements[i] && this.elements[i].data.ty === 0){
            this.elements[i].resize(this.globalData.transformCanvas);
        }
    }*/
    this.ctxTransform(this.transformCanvas.props);
    this.canvasContext.beginPath();
    this.canvasContext.rect(0,0,this.transformCanvas.w,this.transformCanvas.h);
    this.canvasContext.closePath();
    this.canvasContext.clip();

    this.renderFrame(this.renderedFrame, true);
  };

  CanvasRenderer.prototype.destroy = function() {
    if(this.renderConfig.clearCanvas) {
      this.animationItem.wrapper.innerHTML = '';
    }
    let i;
    const len = this.layers ? this.layers.length : 0;
    for (i = len - 1; i >= 0; i-=1) {
      if (this.elements[i]) {
        this.elements[i].destroy();
      }
    }
    this.elements.length = 0;
    this.globalData.canvasContext = null;
    this.animationItem.container = null;
    this.destroyed = true;
  };

  CanvasRenderer.prototype.renderFrame = function(num, forceRender) {
    if((this.renderedFrame === num && this.renderConfig.clearCanvas === true && !forceRender) || this.destroyed || num === -1){
      return;
    }
    this.renderedFrame = num;
    this.globalData.frameNum = num - this.animationItem._isFirstFrame;
    this.globalData.frameId += 1;
    this.globalData._mdf = !this.renderConfig.clearCanvas || forceRender;
    this.globalData.projectInterface.currentFrame = num;

    // console.log('--------');
    // console.log('NEW: ',num);
    let i;
    const len = this.layers.length;
    if(!this.completeLayers){
      this.checkLayers(num);
    }

    for (i = 0; i < len; i++) {
      if (this.completeLayers || this.elements[i]) {
        this.elements[i].prepareFrame(num - this.layers[i].st);
      }
    }
    if(this.globalData._mdf) {
      if (this.renderConfig.clearCanvas === true) {
        this.canvasContext.clearRect(
            0, 0, this.transformCanvas.w, this.transformCanvas.h);
      } else {
        this.save();
      }
      for (i = len - 1; i >= 0; i -= 1) {
        if (this.completeLayers || this.elements[i]) {
          this.elements[i].renderFrame();
        }
      }
      if (this.renderConfig.clearCanvas !== true) {
        this.restore();
      }
    }
  };

  CanvasRenderer.prototype.buildItem = function(pos) {
    const elements = this.elements;
    if(elements[pos] || this.layers[pos].ty == 99){
      return;
    }
    const element = this.createItem(this.layers[pos], this, this.globalData);
    elements[pos] = element;
    element.initExpressions();
    /*if(this.layers[pos].ty === 0){
        element.resize(this.globalData.transformCanvas);
    }*/
  };

  CanvasRenderer.prototype.checkPendingElements = function() {
    while(this.pendingElements.length){
      const element = this.pendingElements.pop();
      element.checkParenting();
    }
  };

  CanvasRenderer.prototype.hide = function() {
    this.animationItem.container.style.display = 'none';
  };

  CanvasRenderer.prototype.show = function() {
    this.animationItem.container.style.display = 'block';
  };

  CanvasRenderer.prototype.configAnimation = function(animData) {
    if(this.animationItem.wrapper){
      this.animationItem.container = createTag('canvas');
      this.animationItem.container.style.width = '100%';
      this.animationItem.container.style.height = '100%';
      // this.animationItem.container.style.transform = 'translate3d(0,0,0)';
      // this.animationItem.container.style.webkitTransform =
      // 'translate3d(0,0,0)';
      this.animationItem.container.style.transformOrigin =
          this.animationItem.container.style.mozTransformOrigin =
              this.animationItem.container.style.webkitTransformOrigin =
                  this.animationItem.container.style['-webkit-transform'] =
                      '0px 0px 0px';
      this.animationItem.wrapper.appendChild(this.animationItem.container);
      this.canvasContext = this.animationItem.container.getContext('2d');
      if (this.renderConfig.className) {
        this.animationItem.container.setAttribute(
            'class', this.renderConfig.className);
      }
    }else{
      this.canvasContext = this.renderConfig.context;
    }
    this.data = animData;
    this.layers = animData.layers;
    this.transformCanvas = {
      w: animData.w,
      h: animData.h,
      sx: 0,
      sy: 0,
      tx: 0,
      ty: 0,
    };
    this.globalData.frameId = 0;
    this.globalData.frameRate = animData.fr;
    this.globalData.nm = animData.nm;
    this.globalData.compSize = {
      w: animData.w,
      h: animData.h,
    };
    this.globalData.canvasContext = this.canvasContext;
    this.globalData.renderer = this;
    this.globalData.isDashed = false;
    this.globalData.progressiveLoad = this.renderConfig.progressiveLoad;
    this.globalData.transformCanvas = this.transformCanvas;
    this.elements = createSizedArray(animData.layers.length);

    this.updateContainerSize();
  };

  function MaskElement(data, element, globalData) {
    this.data = data;
    this.element = element;
    this.globalData = globalData;
    this.storedData = [];
    this.masksProperties = this.data.masksProperties || [];
    this.maskElement = null;
    const defs = this.globalData.defs;
    let i;
    let len = this.masksProperties ? this.masksProperties.length : 0;
    this.viewData = createSizedArray(len);
    this.solidPath = '';


    let path;
    const properties = this.masksProperties;
    let count = 0;
    const currentMasks = [];
    let j;
    let jLen;
    const layerId = createElementID();
    let rect;
    let expansor;
    let feMorph;
    let x;
    let maskType = 'clipPath';
    let maskRef = 'clip-path';
    for (i = 0; i < len; i++) {
      if ((properties[i].mode !== 'a' && properties[i].mode !== 'n') ||
          properties[i].inv || properties[i].o.k !== 100) {
        maskType = 'mask';
        maskRef = 'mask';
      }

      if ((properties[i].mode == 's' || properties[i].mode == 'i') &&
          count === 0) {
        rect = createNS('rect');
        rect.setAttribute('fill', '#ffffff');
        rect.setAttribute('width', this.element.comp.data.w || 0);
        rect.setAttribute('height', this.element.comp.data.h || 0);
        currentMasks.push(rect);
      } else {
        rect = null;
      }

      path = createNS('path');
      if (properties[i].mode == 'n') {
        // TODO move this to a factory or to a constructor
        this.viewData[i] = {
          op: PropertyFactory.getProp(
              this.element, properties[i].o, 0, 0.01, this.element),
          prop:
              ShapePropertyFactory.getShapeProp(this.element, properties[i], 3),
          elem: path,
          lastPath: '',
        };
        defs.appendChild(path);
        continue;
      }
      count += 1;

      path.setAttribute(
          'fill', properties[i].mode === 's' ? '#000000' : '#ffffff');
      path.setAttribute('clip-rule', 'nonzero');
      let filterID;

      if (properties[i].x.k !== 0) {
        maskType = 'mask';
        maskRef = 'mask';
        x = PropertyFactory.getProp(
            this.element, properties[i].x, 0, null, this.element);
        filterID = createElementID();
        expansor = createNS('filter');
        expansor.setAttribute('id', filterID);
        feMorph = createNS('feMorphology');
        feMorph.setAttribute('operator', 'erode');
        feMorph.setAttribute('in', 'SourceGraphic');
        feMorph.setAttribute('radius', '0');
        expansor.appendChild(feMorph);
        defs.appendChild(expansor);
        path.setAttribute(
            'stroke', properties[i].mode === 's' ? '#000000' : '#ffffff');
      } else {
        feMorph = null;
        x = null;
      }

      // TODO move this to a factory or to a constructor
      this.storedData[i] = {
        elem: path,
        x: x,
        expan: feMorph,
        lastPath: '',
        lastOperator: '',
        filterId: filterID,
        lastRadius: 0,
      };
      if (properties[i].mode == 'i') {
        jLen = currentMasks.length;
        const g = createNS('g');
        for (j = 0; j < jLen; j += 1) {
          g.appendChild(currentMasks[j]);
        }
        const mask = createNS('mask');
        mask.setAttribute('mask-type', 'alpha');
        mask.setAttribute('id', layerId + '_' + count);
        mask.appendChild(path);
        defs.appendChild(mask);
        g.setAttribute(
            'mask', 'url(' + locationHref + '#' + layerId + '_' + count + ')');

        currentMasks.length = 0;
        currentMasks.push(g);
      } else {
        currentMasks.push(path);
      }
      if (properties[i].inv && !this.solidPath) {
        this.solidPath = this.createLayerSolidPath();
      }
      // TODO move this to a factory or to a constructor
      this.viewData[i] = {
        elem: path,
        lastPath: '',
        op: PropertyFactory.getProp(
            this.element, properties[i].o, 0, 0.01, this.element),
        prop: ShapePropertyFactory.getShapeProp(this.element, properties[i], 3),
        invRect: rect,
      };
      if (!this.viewData[i].prop.k) {
        this.drawPath(properties[i], this.viewData[i].prop.v, this.viewData[i]);
      }
    }

    this.maskElement = createNS( maskType);

    len = currentMasks.length;
    for(i=0;i<len;i+=1){
      this.maskElement.appendChild(currentMasks[i]);
    }

    if(count > 0){
      this.maskElement.setAttribute('id', layerId);
      this.element.maskedElement.setAttribute(
          maskRef, 'url(' + locationHref + '#' + layerId + ')');
      defs.appendChild(this.maskElement);
    }
    if (this.viewData.length) {
      this.element.addRenderableComponent(this);
    }
  }

  MaskElement.prototype.getMaskProperty = function(pos) {
    return this.viewData[pos].prop;
  };

  MaskElement.prototype.renderFrame = function(isFirstFrame) {
    const finalMat = this.element.finalTransform.mat;
    let i;
    const len = this.masksProperties.length;
    for (i = 0; i < len; i++) {
      if (this.viewData[i].prop._mdf || isFirstFrame) {
        this.drawPath(
            this.masksProperties[i], this.viewData[i].prop.v, this.viewData[i]);
      }
      if (this.viewData[i].op._mdf || isFirstFrame) {
        this.viewData[i].elem.setAttribute(
            'fill-opacity', this.viewData[i].op.v);
      }
      if (this.masksProperties[i].mode !== 'n') {
        if (this.viewData[i].invRect &&
            (this.element.finalTransform.mProp._mdf || isFirstFrame)) {
          this.viewData[i].invRect.setAttribute('x', -finalMat.props[12]);
          this.viewData[i].invRect.setAttribute('y', -finalMat.props[13]);
        }
        if (this.storedData[i].x &&
            (this.storedData[i].x._mdf || isFirstFrame)) {
          const feMorph = this.storedData[i].expan;
          if (this.storedData[i].x.v < 0) {
            if (this.storedData[i].lastOperator !== 'erode') {
              this.storedData[i].lastOperator = 'erode';
              this.storedData[i].elem.setAttribute(
                  'filter',
                  'url(' + locationHref + '#' + this.storedData[i].filterId +
                      ')');
            }
            feMorph.setAttribute('radius', -this.storedData[i].x.v);
          } else {
            if (this.storedData[i].lastOperator !== 'dilate') {
              this.storedData[i].lastOperator = 'dilate';
              this.storedData[i].elem.setAttribute('filter', null);
            }
            this.storedData[i].elem.setAttribute(
                'stroke-width', this.storedData[i].x.v * 2);
          }
        }
      }
    }
  };

  MaskElement.prototype.getMaskelement = function() {
    return this.maskElement;
  };

  MaskElement.prototype.createLayerSolidPath = function() {
    let path = 'M0,0 ';
    path += ' h' + this.globalData.compSize.w ;
    path += ' v' + this.globalData.compSize.h ;
    path += ' h-' + this.globalData.compSize.w ;
    path += ' v-' + this.globalData.compSize.h + ' ';
    return path;
  };

  MaskElement.prototype.drawPath = function(pathData, pathNodes, viewData) {
    let pathString = ' M' + pathNodes.v[0][0] + ',' + pathNodes.v[0][1];
    let i;
    const len = pathNodes._length;
    for(i=1;i<len;i+=1){
      // pathString += " C"+pathNodes.o[i-1][0]+','+pathNodes.o[i-1][1] + "
      // "+pathNodes.i[i][0]+','+pathNodes.i[i][1] + "
      // "+pathNodes.v[i][0]+','+pathNodes.v[i][1];
      pathString += ' C' + pathNodes.o[i - 1][0] + ',' + pathNodes.o[i - 1][1] +
          ' ' + pathNodes.i[i][0] + ',' + pathNodes.i[i][1] + ' ' +
          pathNodes.v[i][0] + ',' + pathNodes.v[i][1];
    }
    // pathString += " C"+pathNodes.o[i-1][0]+','+pathNodes.o[i-1][1] + "
    // "+pathNodes.i[0][0]+','+pathNodes.i[0][1] + "
    // "+pathNodes.v[0][0]+','+pathNodes.v[0][1];
    if(pathNodes.c && len > 1){
      pathString += ' C' + pathNodes.o[i - 1][0] + ',' + pathNodes.o[i - 1][1] +
          ' ' + pathNodes.i[0][0] + ',' + pathNodes.i[0][1] + ' ' +
          pathNodes.v[0][0] + ',' + pathNodes.v[0][1];
    }
    //pathNodes.__renderedString = pathString;

    if(viewData.lastPath !== pathString){
      let pathShapeValue = '';
      if (viewData.elem) {
        if (pathNodes.c) {
          pathShapeValue =
              pathData.inv ? this.solidPath + pathString : pathString;
        }
        viewData.elem.setAttribute('d', pathShapeValue);
      }
      viewData.lastPath = pathString;
    }
  };

  MaskElement.prototype.destroy = function() {
    this.element = null;
    this.globalData = null;
    this.maskElement = null;
    this.data = null;
    this.masksProperties = null;
  };

  /**
   * @file
   * Handles AE's layer parenting property.
   *
   */

  function HierarchyElement() {}

  HierarchyElement.prototype = {
    /**
     * @function
     * Initializes hierarchy properties
     *
     */
    initHierarchy: function() {
      // element's parent list
      this.hierarchy = [];
      //if element is parent of another layer _isParent will be true
      this._isParent = false;
      this.checkParenting();
    },
    /**
     * @function
     * Sets layer's hierarchy.
     * @param {array} hierarch
     * layer's parent list
     *
     */
    setHierarchy: function(hierarchy) {
      this.hierarchy = hierarchy;
    },
    /**
     * @function
     * Sets layer as parent.
     *
     */
    setAsParent: function() {
      this._isParent = true;
    },
    /**
     * @function
     * Searches layer's parenting chain
     *
     */
    checkParenting: function() {
      if (this.data.parent !== undefined){
        this.comp.buildElementParenting(this, this.data.parent, []);
      }
    },
  };
  /**
   * @file
   * Handles element's layer frame update.
   * Checks layer in point and out point
   *
   */

  function FrameElement() {}

  FrameElement.prototype = {
    /**
     * @function
     * Initializes frame related properties.
     *
     */
    initFrame: function() {
      // set to true when inpoint is rendered
      this._isFirstFrame = false;
      // list of animated properties
      this.dynamicProperties = [];
      // If layer has been modified in current tick this will be true
      this._mdf = false;
    },
    /**
     * @function
     * Calculates all dynamic values
     *
     * @param {number} num
     * current frame number in Layer's time
     * @param {boolean} isVisible
     * if layers is currently in range
     *
     */
    prepareProperties: function(num, isVisible) {
      let i;
      const len = this.dynamicProperties.length;
      for (i = 0; i < len; i += 1) {
        if (isVisible ||
            (this._isParent &&
             this.dynamicProperties[i].propType === 'transform')) {
          this.dynamicProperties[i].getValue();
          if (this.dynamicProperties[i]._mdf) {
            this.globalData._mdf = true;
            this._mdf = true;
          }
        }
      }
    },
    addDynamicProperty: function(prop) {
      if (this.dynamicProperties.indexOf(prop) === -1) {
        this.dynamicProperties.push(prop);
      }
    },
  };
  function TransformElement() {}

  TransformElement.prototype = {
    initTransform: function() {
      this.finalTransform = {
        mProp: this.data.ks ? TransformPropertyFactory.getTransformProperty(
                                  this, this.data.ks, this) :
                              {o: 0},
        _matMdf: false,
        _opMdf: false,
        mat: new Matrix(),
      };
      if (this.data.ao) {
        this.finalTransform.mProp.autoOriented = true;
      }

      // TODO: check TYPE 11: Guided elements
      if (this.data.ty !== 11) {
        // this.createElements();
      }
    },
    renderTransform: function() {
      this.finalTransform._opMdf =
          this.finalTransform.mProp.o._mdf || this._isFirstFrame;
      this.finalTransform._matMdf =
          this.finalTransform.mProp._mdf || this._isFirstFrame;

      if (this.hierarchy) {
        let mat;
        const finalMat = this.finalTransform.mat;
        let i = 0;
        const len = this.hierarchy.length;
        // Checking if any of the transformation matrices in the hierarchy chain
        // has changed.
        if (!this.finalTransform._matMdf) {
          while (i < len) {
            if (this.hierarchy[i].finalTransform.mProp._mdf) {
              this.finalTransform._matMdf = true;
              break;
            }
            i += 1;
          }
        }

        if (this.finalTransform._matMdf) {
          mat = this.finalTransform.mProp.v.props;
          finalMat.cloneFromProps(mat);
          for (i = 0; i < len; i += 1) {
            mat = this.hierarchy[i].finalTransform.mProp.v.props;
            finalMat.transform(
                mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6], mat[7],
                mat[8], mat[9], mat[10], mat[11], mat[12], mat[13], mat[14],
                mat[15]);
          }
        }
      }
    },
    globalToLocal: function(pt) {
      const transforms = [];
      transforms.push(this.finalTransform);
      let flag = true;
      let comp = this.comp;
      while (flag) {
        if (comp.finalTransform) {
          if (comp.data.hasMask) {
            transforms.splice(0, 0, comp.finalTransform);
          }
          comp = comp.comp;
        } else {
          flag = false;
        }
      }
      let i;
      const len = transforms.length;
      let ptNew;
      for (i = 0; i < len; i += 1) {
        ptNew = transforms[i].mat.applyToPointArray(0, 0, 0);
        // ptNew = transforms[i].mat.applyToPointArray(pt[0],pt[1],pt[2]);
        pt = [pt[0] - ptNew[0], pt[1] - ptNew[1], 0];
      }
      return pt;
    },
    mHelper: new Matrix(),
  };
  function RenderableElement() {}

  RenderableElement.prototype = {
    initRenderable: function() {
      // layer's visibility related to inpoint and outpoint. Rename isVisible to
      // isInRange
      this.isInRange = false;
      // layer's display state
      this.hidden = false;
      // If layer's transparency equals 0, it can be hidden
      this.isTransparent = false;
      // list of animated components
      this.renderableComponents = [];
    },
    addRenderableComponent: function(component) {
      if (this.renderableComponents.indexOf(component) === -1) {
        this.renderableComponents.push(component);
      }
    },
    removeRenderableComponent: function(component) {
      if (this.renderableComponents.indexOf(component) !== -1) {
        this.renderableComponents.splice(
            this.renderableComponents.indexOf(component), 1);
      }
    },
    prepareRenderableFrame: function(num) {
      this.checkLayerLimits(num);
    },
    checkTransparency: function() {
      if (this.finalTransform.mProp.o.v <= 0) {
        if (!this.isTransparent &&
            this.globalData.renderConfig.hideOnTransparent) {
          this.isTransparent = true;
          this.hide();
        }
      } else if (this.isTransparent) {
        this.isTransparent = false;
        this.show();
      }
    },
    /**
     * @function
     * Initializes frame related properties.
     *
     * @param {number} num
     * current frame number in Layer's time
     *
     */
    checkLayerLimits: function(num) {
      if (this.data.ip - this.data.st <= num &&
          this.data.op - this.data.st > num) {
        if (this.isInRange !== true) {
          this.globalData._mdf = true;
          this._mdf = true;
          this.isInRange = true;
          this.show();
        }
      } else {
        if (this.isInRange !== false) {
          this.globalData._mdf = true;
          this.isInRange = false;
          this.hide();
        }
      }
    },
    renderRenderable: function() {
      let i;
      const len = this.renderableComponents.length;
      for (i = 0; i < len; i += 1) {
        this.renderableComponents[i].renderFrame(this._isFirstFrame);
      }
      /*this.maskManager.renderFrame(this.finalTransform.mat);
      this.renderableEffectsManager.renderFrame(this._isFirstFrame);*/
    },
    sourceRectAtTime: function() {
      return {
        top: 0,
        left: 0,
        width: 100,
        height: 100,
      };
    },
    getLayerSize: function() {
      if (this.data.ty === 5) {
        return {w: this.data.textData.width, h: this.data.textData.height};
      } else {
        return {w: this.data.width, h: this.data.height};
      }
    },
  };
  function RenderableDOMElement() {}

  (function() {
    const _prototype = {
      initElement: function(data, globalData, comp) {
        this.initFrame();
        this.initBaseData(data, globalData, comp);
        this.initTransform(data, globalData, comp);
        this.initHierarchy();
        this.initRenderable();
        this.initRendererElement();
        this.createContainerElements();
        this.createRenderableComponents();
        this.createContent();
        this.hide();
      },
      hide: function() {
        if (!this.hidden && (!this.isInRange || this.isTransparent)) {
          const elem = this.baseElement || this.layerElement;
          elem.style.display = 'none';
          this.hidden = true;
        }
      },
      show: function() {
        if (this.isInRange && !this.isTransparent) {
          if (!this.data.hd) {
            const elem = this.baseElement || this.layerElement;
            elem.style.display = 'block';
          }
          this.hidden = false;
          this._isFirstFrame = true;
        }
      },
      renderFrame: function() {
        // If it is exported as hidden (data.hd === true) no need to render
        // If it is not visible no need to render
        if (this.data.hd || this.hidden) {
          return;
        }
        this.renderTransform();
        this.renderRenderable();
        this.renderElement();
        this.renderInnerContent();
        if (this._isFirstFrame) {
          this._isFirstFrame = false;
        }
      },
      renderInnerContent: function() {},
      prepareFrame: function(num) {
        this._mdf = false;
        this.prepareRenderableFrame(num);
        this.prepareProperties(num, this.isInRange);
        this.checkTransparency();
      },
      destroy: function() {
        this.innerElem = null;
        this.destroyBaseElement();
      },
    };
    extendPrototype([RenderableElement, createProxyFunction(_prototype)], RenderableDOMElement);
  }());
  function ProcessedElement(element, position) {
    this.elem = element;
    this.pos = position;
  }
  function SVGShapeData(transformers, level, shape) {
    this.caches = [];
    this.styles = [];
    this.transformers = transformers;
    this.lStr = '';
    this.sh = shape;
    this.lvl = level;
    //TODO find if there are some cases where _isAnimated can be false.
    // For now, since shapes add up with other shapes. They have to be calculated every time.
    // One way of finding out is checking if all styles associated to this shape depend only of this shape
    this._isAnimated = !!shape.k;
    // TODO: commenting this for now since all shapes are animated
    let i = 0;
    const len = transformers.length;
    while(i < len) {
      if(transformers[i].mProps.dynamicProperties.length) {
        this._isAnimated = true;
        break;
      }
      i += 1;
    }
  }

  SVGShapeData.prototype.setAsAnimated = function() {
    this._isAnimated = true;
  };
  function ShapeGroupData() {
    this.it = [];
    this.prevViewData = [];
    this.gr = createNS('g');
  }
  function ShapeTransformManager() {
    this.sequences = {};
    this.sequenceList = [];
    this.transform_key_count = 0;
  }

  ShapeTransformManager.prototype = {
    addTransformSequence: function(transforms) {
      let i;
      const len = transforms.length;
      let key = '_';
      for (i = 0; i < len; i += 1) {
        key += transforms[i].transform.key + '_';
      }
      let sequence = this.sequences[key];
      if (!sequence) {
        sequence = {
          transforms: [].concat(transforms),
          finalTransform: new Matrix(),
          _mdf: false,
        };
        this.sequences[key] = sequence;
        this.sequenceList.push(sequence);
      }
      return sequence;
    },
    processSequence: function(sequence, isFirstFrame) {
      let i = 0;
      const len = sequence.transforms.length;
      let _mdf = isFirstFrame;
      while (i < len && !isFirstFrame) {
        if (sequence.transforms[i].transform.mProps._mdf) {
          _mdf = true;
          break;
        }
        i += 1;
      }
      if (_mdf) {
        let props;
        sequence.finalTransform.reset();
        for (i = len - 1; i >= 0; i -= 1) {
          props = sequence.transforms[i].transform.mProps.v.props;
          sequence.finalTransform.transform(
              props[0], props[1], props[2], props[3], props[4], props[5],
              props[6], props[7], props[8], props[9], props[10], props[11],
              props[12], props[13], props[14], props[15]);
        }
      }
      sequence._mdf = _mdf;
    },
    processSequences: function(isFirstFrame) {
      let i;
      const len = this.sequenceList.length;
      for (i = 0; i < len; i += 1) {
        this.processSequence(this.sequenceList[i], isFirstFrame);
      }
    },
    getNewKey: function() {
      return '_' + this.transform_key_count++;
    },
  };
  function CVShapeData(element, data, styles, transformsManager) {
    this.styledShapes = [];
    this.tr = [0,0,0,0,0,0];
    let ty = 4;
    if(data.ty == 'rc'){
      ty = 5;
    }else if(data.ty == 'el'){
      ty = 6;
    }else if(data.ty == 'sr'){
      ty = 7;
    }
    this.sh = ShapePropertyFactory.getShapeProp(element,data,ty,element);
    let i;
    const len = styles.length;
    let styledShape;
    for (i = 0; i < len; i += 1) {
      if (!styles[i].closed) {
        styledShape = {
          transforms:
              transformsManager.addTransformSequence(styles[i].transforms),
          trNodes: [],
        };
        this.styledShapes.push(styledShape);
        styles[i].elements.push(styledShape);
      }
    }
  }

  CVShapeData.prototype.setAsAnimated = SVGShapeData.prototype.setAsAnimated;
  function BaseElement() {}

  BaseElement.prototype = {
    checkMasks: function() {
      if (!this.data.hasMask) {
        return false;
      }
      let i = 0;
      const len = this.data.masksProperties.length;
      while (i < len) {
        if ((this.data.masksProperties[i].mode !== 'n' &&
             this.data.masksProperties[i].cl !== false)) {
          return true;
        }
        i += 1;
      }
      return false;
    },
    initExpressions: function() {
      this.layerInterface = LayerExpressionInterface(this);
      if (this.data.hasMask && this.maskManager) {
        this.layerInterface.registerMaskInterface(this.maskManager);
      }
      const effectsInterface =
          EffectsExpressionInterface.createEffectsInterface(
              this, this.layerInterface);
      this.layerInterface.registerEffectsInterface(effectsInterface);

      if (this.data.ty === 0 || this.data.xt) {
        this.compInterface = CompExpressionInterface(this);
      } else if (this.data.ty === 4) {
        this.layerInterface.shapeInterface = ShapeExpressionInterface(
            this.shapesData, this.itemsData, this.layerInterface);
        this.layerInterface.content = this.layerInterface.shapeInterface;
      } else if (this.data.ty === 5) {
        this.layerInterface.textInterface = TextExpressionInterface(this);
        this.layerInterface.text = this.layerInterface.textInterface;
      }
    },
    setBlendMode: function() {
      const blendModeValue = getBlendMode(this.data.bm);
      const elem = this.baseElement || this.layerElement;

      elem.style['mix-blend-mode'] = blendModeValue;
    },
    initBaseData: function(data, globalData, comp) {
      this.globalData = globalData;
      this.comp = comp;
      this.data = data;
      this.layerId = createElementID();

      // Stretch factor for old animations missing this property.
      if (!this.data.sr) {
        this.data.sr = 1;
      }
      // effects manager
      this.effectsManager =
          new EffectsManager(this.data, this, this.dynamicProperties);
    },
    getType: function() {
      return this.type;
    },
    sourceRectAtTime: function() {},
  };
  function NullElement(data, globalData, comp) {
    this.initFrame();
    this.initBaseData(data, globalData, comp);
    this.initFrame();
    this.initTransform(data, globalData, comp);
    this.initHierarchy();
  }

  NullElement.prototype.prepareFrame = function(num) {
    this.prepareProperties(num, true);
  };

  NullElement.prototype.renderFrame = function() {};

  NullElement.prototype.getBaseElement = function() {
    return null;
  };

  NullElement.prototype.destroy = function() {};

  NullElement.prototype.sourceRectAtTime = function() {};

  NullElement.prototype.hide = function() {};

  extendPrototype(
      [BaseElement, TransformElement, HierarchyElement, FrameElement],
      NullElement);

  function SVGBaseElement() {}

  SVGBaseElement.prototype =
      {
        initRendererElement: function() {
          this.layerElement = createNS('g');
        },
        createContainerElements: function() {
          this.matteElement = createNS('g');
          this.transformedElement = this.layerElement;
          this.maskedElement = this.layerElement;
          this._sizeChanged = false;
          let layerElementParent = null;
          // If this layer acts as a mask for the following layer
          let filId;
          let fil;
          let gg;
          if (this.data.td) {
            if (this.data.td == 3 || this.data.td == 1) {
              const masker = createNS('mask');
              masker.setAttribute('id', this.layerId);
              masker.setAttribute(
                  'mask-type', this.data.td == 3 ? 'luminance' : 'alpha');
              masker.appendChild(this.layerElement);
              layerElementParent = masker;
              this.globalData.defs.appendChild(masker);
              // This is only for IE and Edge when mask if of type alpha
              if (!featureSupport.maskType && this.data.td == 1) {
                masker.setAttribute('mask-type', 'luminance');
                filId = createElementID();
                fil = filtersFactory.createFilter(filId);
                this.globalData.defs.appendChild(fil);
                fil.appendChild(filtersFactory.createAlphaToLuminanceFilter());
                gg = createNS('g');
                gg.appendChild(this.layerElement);
                layerElementParent = gg;
                masker.appendChild(gg);
                gg.setAttribute(
                    'filter', 'url(' + locationHref + '#' + filId + ')');
              }
            } else if (this.data.td == 2) {
              const maskGroup = createNS('mask');
              maskGroup.setAttribute('id', this.layerId);
              maskGroup.setAttribute('mask-type', 'alpha');
              const maskGrouper = createNS('g');
              maskGroup.appendChild(maskGrouper);
              filId = createElementID();
              fil = filtersFactory.createFilter(filId);
              ////

              // This solution doesn't work on Android when meta tag with
              // viewport attribute is set
              /*let feColorMatrix = createNS('feColorMatrix');
              feColorMatrix.setAttribute('type', 'matrix');
              feColorMatrix.setAttribute('color-interpolation-filters', 'sRGB');
              feColorMatrix.setAttribute('values','1 0 0 0 0 0 1 0 0 0 0 0 1 0 0
              0 0 0 -1 1'); fil.appendChild(feColorMatrix);*/
              ////
              const feCTr = createNS('feComponentTransfer');
              feCTr.setAttribute('in', 'SourceGraphic');
              fil.appendChild(feCTr);
              const feFunc = createNS('feFuncA');
              feFunc.setAttribute('type', 'table');
              feFunc.setAttribute('tableValues', '1.0 0.0');
              feCTr.appendChild(feFunc);
              ////
              this.globalData.defs.appendChild(fil);
              const alphaRect = createNS('rect');
              alphaRect.setAttribute('width', this.comp.data.w);
              alphaRect.setAttribute('height', this.comp.data.h);
              alphaRect.setAttribute('x', '0');
              alphaRect.setAttribute('y', '0');
              alphaRect.setAttribute('fill', '#ffffff');
              alphaRect.setAttribute('opacity', '0');
              maskGrouper.setAttribute(
                  'filter', 'url(' + locationHref + '#' + filId + ')');
              maskGrouper.appendChild(alphaRect);
              maskGrouper.appendChild(this.layerElement);
              layerElementParent = maskGrouper;
              if (!featureSupport.maskType) {
                maskGroup.setAttribute('mask-type', 'luminance');
                fil.appendChild(filtersFactory.createAlphaToLuminanceFilter());
                gg = createNS('g');
                maskGrouper.appendChild(alphaRect);
                gg.appendChild(this.layerElement);
                layerElementParent = gg;
                maskGrouper.appendChild(gg);
              }
              this.globalData.defs.appendChild(maskGroup);
            }
          } else if (this.data.tt) {
            this.matteElement.appendChild(this.layerElement);
            layerElementParent = this.matteElement;
            this.baseElement = this.matteElement;
          } else {
            this.baseElement = this.layerElement;
          }
          if (this.data.ln) {
            this.layerElement.setAttribute('id', this.data.ln);
          }
          if (this.data.cl) {
            this.layerElement.setAttribute('class', this.data.cl);
          }
          // Clipping compositions to hide content that exceeds boundaries. If
          // collapsed transformations is on, component should not be clipped
          if (this.data.ty === 0 && !this.data.hd) {
            const cp = createNS('clipPath');
            const pt = createNS('path');
            pt.setAttribute('d','M0,0 L' + this.data.w + ',0' + ' L' + this.data.w + ',' + this.data.h + ' L0,' + this.data.h + 'z');
            const clipId = createElementID();
            cp.setAttribute('id',clipId);
            cp.appendChild(pt);
            this.globalData.defs.appendChild(cp);

            if (this.checkMasks()) {
              const cpGroup = createNS('g');
              cpGroup.setAttribute(
                  'clip-path', 'url(' + locationHref + '#' + clipId + ')');
              cpGroup.appendChild(this.layerElement);
              this.transformedElement = cpGroup;
              if (layerElementParent) {
                layerElementParent.appendChild(this.transformedElement);
              } else {
                this.baseElement = this.transformedElement;
              }
            } else {
              this.layerElement.setAttribute(
                  'clip-path', 'url(' + locationHref + '#' + clipId + ')');
            }
          }
          if (this.data.bm !== 0) {
            this.setBlendMode();
          }
        },
        renderElement: function() {
          if (this.finalTransform._matMdf) {
            this.transformedElement.setAttribute('transform', this.finalTransform.mat.to2dCSS());
          }
          if (this.finalTransform._opMdf) {
            this.transformedElement.setAttribute('opacity', this.finalTransform.mProp.o.v);
          }
        },
        destroyBaseElement: function() {
          this.layerElement = null;
          this.matteElement = null;
          this.maskManager.destroy();
        },
        getBaseElement: function() {
          if (this.data.hd) {
            return null;
          }
          return this.baseElement;
        },
        createRenderableComponents: function() {
          this.maskManager = new MaskElement(this.data, this, this.globalData);
          this.renderableEffectsManager = new SVGEffects(this);
        },
        setMatte: function(id) {
          if (!this.matteElement) {
            return;
          }
          this.matteElement.setAttribute(
              'mask', 'url(' + locationHref + '#' + id + ')');
        },
      };
  function IShapeElement() {}

  IShapeElement.prototype = {
    addShapeToModifiers: function(data) {
      let i;
      const len = this.shapeModifiers.length;
      for (i = 0; i < len; i += 1) {
        this.shapeModifiers[i].addShape(data);
      }
    },
    isShapeInAnimatedModifiers: function(data) {
      const i = 0;
      const len = this.shapeModifiers.length;
      while (i < len) {
        if (this.shapeModifiers[i].isAnimatedWithShape(data)) {
          return true;
        }
      }
      return false;
    },
    renderModifiers: function() {
      if (!this.shapeModifiers.length) {
        return;
      }
      let i;
      let len = this.shapes.length;
      for (i = 0; i < len; i += 1) {
        this.shapes[i].sh.reset();
      }

      len = this.shapeModifiers.length;
      for (i = len - 1; i >= 0; i -= 1) {
        this.shapeModifiers[i].processShapes(this._isFirstFrame);
      }
    },
    lcEnum: {
      '1': 'butt',
      '2': 'round',
      '3': 'square',
    },
    ljEnum: {
      '1': 'miter',
      '2': 'round',
      '3': 'bevel',
    },
    searchProcessedElement: function(elem) {
      const elements = this.processedElements;
      let i = 0;
      const len = elements.length;
      while (i < len) {
        if (elements[i].elem === elem) {
          return elements[i].pos;
        }
        i += 1;
      }
      return 0;
    },
    addProcessedElement: function(elem, pos) {
      const elements = this.processedElements;
      let i = elements.length;
      while (i) {
        i -= 1;
        if (elements[i].elem === elem) {
          elements[i].pos = pos;
          return;
        }
      }
      elements.push(new ProcessedElement(elem, pos));
    },
    prepareFrame: function(num) {
      this.prepareRenderableFrame(num);
      this.prepareProperties(num, this.isInRange);
    },
  };
  function ITextElement() {}

  ITextElement.prototype.initElement = function(data, globalData, comp) {
    this.lettersChangedFlag = true;
    this.initFrame();
    this.initBaseData(data, globalData, comp);
    this.textProperty = new TextProperty(this, data.t, this.dynamicProperties);
    this.textAnimator = new TextAnimatorProperty(data.t, this.renderType, this);
    this.initTransform(data, globalData, comp);
    this.initHierarchy();
    this.initRenderable();
    this.initRendererElement();
    this.createContainerElements();
    this.createRenderableComponents();
    this.createContent();
    this.hide();
    this.textAnimator.searchProperties(this.dynamicProperties);
  };

  ITextElement.prototype.prepareFrame = function(num) {
    this._mdf = false;
    this.prepareRenderableFrame(num);
    this.prepareProperties(num, this.isInRange);
    if(this.textProperty._mdf || this.textProperty._isFirstFrame) {
      this.buildNewText();
      this.textProperty._isFirstFrame = false;
      this.textProperty._mdf = false;
    }
  };

  ITextElement.prototype.createPathShape = function(matrixHelper, shapes) {
    let j;
    const jLen = shapes.length;
    let k;
    let pathNodes;
    let shapeStr = '';
    for(j=0;j<jLen;j+=1){
      pathNodes = shapes[j].ks.k;
      shapeStr +=
          buildShapeString(pathNodes, pathNodes.i.length, true, matrixHelper);
    }
    return shapeStr;
  };

  ITextElement.prototype.updateDocumentData = function(newData, index) {
    this.textProperty.updateDocumentData(newData, index);
  };

  ITextElement.prototype.canResizeFont = function(_canResize) {
    this.textProperty.canResizeFont(_canResize);
  };

  ITextElement.prototype.setMinimumFontSize = function(_fontSize) {
    this.textProperty.setMinimumFontSize(_fontSize);
  };

  ITextElement.prototype.applyTextPropertiesToMatrix = function(
      documentData, matrixHelper, lineNumber, xPos, yPos) {
    if(documentData.ps){
      matrixHelper.translate(
          documentData.ps[0], documentData.ps[1] + documentData.ascent, 0);
    }
    matrixHelper.translate(0,-documentData.ls,0);
    switch(documentData.j){
      case 1:
        matrixHelper.translate(
            documentData.justifyOffset +
                (documentData.boxWidth - documentData.lineWidths[lineNumber]),
            0, 0);
        break;
      case 2:
        matrixHelper.translate(
            documentData.justifyOffset +
                (documentData.boxWidth - documentData.lineWidths[lineNumber]) /
                    2,
            0, 0);
        break;
    }
    matrixHelper.translate(xPos, yPos, 0);
  };


  ITextElement.prototype.buildColor = function(colorData) {
    return 'rgb(' + Math.round(colorData[0]*255) + ',' + Math.round(colorData[1]*255) + ',' + Math.round(colorData[2]*255) + ')';
  };

  ITextElement.prototype.emptyProp = new LetterProps();

  ITextElement.prototype.destroy = function() {

  };
  function ICompElement() {}

  extendPrototype(
      [
        BaseElement,
        TransformElement,
        HierarchyElement,
        FrameElement,
        RenderableDOMElement,
      ],
      ICompElement);

  ICompElement.prototype.initElement = function(data, globalData, comp) {
    this.initFrame();
    this.initBaseData(data, globalData, comp);
    this.initTransform(data, globalData, comp);
    this.initRenderable();
    this.initHierarchy();
    this.initRendererElement();
    this.createContainerElements();
    this.createRenderableComponents();
    if(this.data.xt || !globalData.progressiveLoad){
      this.buildAllItems();
    }
    this.hide();
  };

  /*ICompElement.prototype.hide = function(){
      if(!this.hidden){
          this.hideElement();
          let i,len = this.elements.length;
          for( i = 0; i < len; i+=1 ){
              if(this.elements[i]){
                  this.elements[i].hide();
              }
          }
      }
  };*/

  ICompElement.prototype.prepareFrame = function(num) {
    this._mdf = false;
    this.prepareRenderableFrame(num);
    this.prepareProperties(num, this.isInRange);
    if(!this.isInRange && !this.data.xt){
      return;
    }

    if (!this.tm._placeholder) {
      let timeRemapped = this.tm.v;
      if (timeRemapped === this.data.op) {
        timeRemapped = this.data.op - 1;
      }
      this.renderedFrame = timeRemapped;
    } else {
      this.renderedFrame = num / this.data.sr;
    }
    let i;
    const len = this.elements.length;
    if(!this.completeLayers){
      this.checkLayers(this.renderedFrame);
    }
    //This iteration needs to be backwards because of how expressions connect between each other
    for( i = len - 1; i >= 0; i -= 1 ){
      if (this.completeLayers || this.elements[i]) {
        this.elements[i].prepareFrame(this.renderedFrame - this.layers[i].st);
        if (this.elements[i]._mdf) {
          this._mdf = true;
        }
      }
    }
  };

  ICompElement.prototype.renderInnerContent = function() {
    let i;
    const len = this.layers.length;
    for( i = 0; i < len; i += 1 ){
      if (this.completeLayers || this.elements[i]) {
        this.elements[i].renderFrame();
      }
    }
  };

  ICompElement.prototype.setElements = function(elems) {
    this.elements = elems;
  };

  ICompElement.prototype.getElements = function() {
    return this.elements;
  };

  ICompElement.prototype.destroyElements = function() {
    let i;
    const len = this.layers.length;
    for( i = 0; i < len; i+=1 ){
      if (this.elements[i]) {
        this.elements[i].destroy();
      }
    }
  };

  ICompElement.prototype.destroy = function() {
    this.destroyElements();
    this.destroyBaseElement();
  };

  function IImageElement(data, globalData, comp) {
    this.assetData = globalData.getAssetData(data.refId);
    this.initElement(data,globalData,comp);
    this.sourceRect = {top:0,left:0,width:this.assetData.w,height:this.assetData.h};
  }

  extendPrototype(
      [
        BaseElement,
        TransformElement,
        SVGBaseElement,
        HierarchyElement,
        FrameElement,
        RenderableDOMElement,
      ],
      IImageElement);

  IImageElement.prototype.createContent = function() {
    const assetPath = this.globalData.getAssetsPath(this.assetData);

    this.innerElem = createNS('image');
    this.innerElem.setAttribute('width', this.assetData.w + 'px');
    this.innerElem.setAttribute('height', this.assetData.h + 'px');
    this.innerElem.setAttribute('preserveAspectRatio',this.assetData.pr || this.globalData.renderConfig.imagePreserveAspectRatio);
    this.innerElem.setAttributeNS('http://www.w3.org/1999/xlink','href',assetPath);

    this.layerElement.appendChild(this.innerElem);
  };

  IImageElement.prototype.sourceRectAtTime = function() {
    return this.sourceRect;
  };
  function ISolidElement(data, globalData, comp) {
    this.initElement(data,globalData,comp);
  }
  extendPrototype([IImageElement], ISolidElement);

  ISolidElement.prototype.createContent = function() {
    const rect = createNS('rect');
    ////rect.style.width = this.data.sw;
    ////rect.style.height = this.data.sh;
    ////rect.style.fill = this.data.sc;
    rect.setAttribute('width',this.data.sw);
    rect.setAttribute('height',this.data.sh);
    rect.setAttribute('fill',this.data.sc);
    this.layerElement.appendChild(rect);
  };
  function SVGShapeElement(data, globalData, comp) {
    //List of drawable elements
    this.shapes = [];
    // Full shape data
    this.shapesData = data.shapes;
    //List of styles that will be applied to shapes
    this.stylesList = [];
    //List of modifiers that will be applied to shapes
    this.shapeModifiers = [];
    //List of items in shape tree
    this.itemsData = [];
    //List of items in previous shape tree
    this.processedElements = [];
    // List of animated components
    this.animatedContents = [];
    this.initElement(data,globalData,comp);
    //Moving any property that doesn't get too much access after initialization because of v8 way of handling more than 10 properties.
    // List of elements that have been created
    this.prevViewData = [];
    //Moving any property that doesn't get too much access after initialization because of v8 way of handling more than 10 properties.
  }

  extendPrototype(
      [
        BaseElement,
        TransformElement,
        SVGBaseElement,
        IShapeElement,
        HierarchyElement,
        FrameElement,
        RenderableDOMElement,
      ],
      SVGShapeElement);

  SVGShapeElement.prototype.initSecondaryElement = function() {};

  SVGShapeElement.prototype.identityMatrix = new Matrix();

  SVGShapeElement.prototype.buildExpressionInterface = function() {};

  SVGShapeElement.prototype.createContent = function() {
    this.searchShapes(this.shapesData,this.itemsData,this.prevViewData,this.layerElement, 0, [], true);
    this.filterUniqueShapes();
  };

  /*
  This method searches for multiple shapes that affect a single element and one
  of them is animated
  */
  SVGShapeElement.prototype.filterUniqueShapes = function() {
    let i;
    const len = this.shapes.length;
    let shape;
    let j;
    const jLen = this.stylesList.length;
    let style;
    const count = 0;
    const tempShapes = [];
    let areAnimated = false;
    for(j = 0; j < jLen; j += 1) {
      style = this.stylesList[j];
      areAnimated = false;
      tempShapes.length = 0;
      for (i = 0; i < len; i += 1) {
        shape = this.shapes[i];
        if (shape.styles.indexOf(style) !== -1) {
          tempShapes.push(shape);
          areAnimated = shape._isAnimated || areAnimated;
        }
      }
      if (tempShapes.length > 1 && areAnimated) {
        this.setShapesAsAnimated(tempShapes);
      }
    }
  };

  SVGShapeElement.prototype.setShapesAsAnimated = function(shapes) {
    let i;
    const len = shapes.length;
    for(i = 0; i < len; i += 1) {
      shapes[i].setAsAnimated();
    }
  };

  SVGShapeElement.prototype.createStyleElement = function(data, level) {
    //TODO: prevent drawing of hidden styles
    let elementData;
    const styleOb = new SVGStyleData(data, level);

    const pathElement = styleOb.pElem;
    if(data.ty === 'st') {
      elementData = new SVGStrokeStyleData(this, data, styleOb);
    } else if(data.ty === 'fl') {
      elementData = new SVGFillStyleData(this, data, styleOb);
    } else if(data.ty === 'gf' || data.ty === 'gs') {
      const gradientConstructor = data.ty === 'gf' ? SVGGradientFillStyleData :
                                                     SVGGradientStrokeStyleData;
      elementData = new gradientConstructor(this, data, styleOb);
      this.globalData.defs.appendChild(elementData.gf);
      if (elementData.maskId) {
        this.globalData.defs.appendChild(elementData.ms);
        this.globalData.defs.appendChild(elementData.of);
        pathElement.setAttribute(
            'mask', 'url(' + locationHref + '#' + elementData.maskId + ')');
      }
    }

    if(data.ty === 'st' || data.ty === 'gs') {
      pathElement.setAttribute(
          'stroke-linecap', this.lcEnum[data.lc] || 'round');
      pathElement.setAttribute(
          'stroke-linejoin', this.ljEnum[data.lj] || 'round');
      pathElement.setAttribute('fill-opacity', '0');
      if (data.lj === 1) {
        pathElement.setAttribute('stroke-miterlimit', data.ml);
      }
    }

    if(data.r === 2) {
      pathElement.setAttribute('fill-rule', 'evenodd');
    }

    if(data.ln){
      pathElement.setAttribute('id', data.ln);
    }
    if(data.cl){
      pathElement.setAttribute('class', data.cl);
    }
    if(data.bm){
      pathElement.style['mix-blend-mode'] = getBlendMode(data.bm);
    }
    this.stylesList.push(styleOb);
    this.addToAnimatedContents(data, elementData);
    return elementData;
  };

  SVGShapeElement.prototype.createGroupElement = function(data) {
    const elementData = new ShapeGroupData();
    if(data.ln){
      elementData.gr.setAttribute('id', data.ln);
    }
    if(data.cl){
      elementData.gr.setAttribute('class', data.cl);
    }
    if(data.bm){
      elementData.gr.style['mix-blend-mode'] = getBlendMode(data.bm);
    }
    return elementData;
  };

  SVGShapeElement.prototype.createTransformElement = function(data, container) {
    const transformProperty =
        TransformPropertyFactory.getTransformProperty(this, data, this);
    const elementData =
        new SVGTransformData(transformProperty, transformProperty.o, container);
    this.addToAnimatedContents(data, elementData);
    return elementData;
  };

  SVGShapeElement.prototype.createShapeElement = function(
      data, ownTransformers, level) {
    let ty = 4;
    if(data.ty === 'rc'){
      ty = 5;
    }else if(data.ty === 'el'){
      ty = 6;
    }else if(data.ty === 'sr'){
      ty = 7;
    }
    const shapeProperty =
        ShapePropertyFactory.getShapeProp(this, data, ty, this);
    const elementData = new SVGShapeData(ownTransformers, level, shapeProperty);
    this.shapes.push(elementData);
    this.addShapeToModifiers(elementData);
    this.addToAnimatedContents(data, elementData);
    return elementData;
  };

  SVGShapeElement.prototype.addToAnimatedContents = function(data, element) {
    let i = 0;
    const len = this.animatedContents.length;
    while(i < len) {
      if (this.animatedContents[i].element === element) {
        return;
      }
      i += 1;
    }
    this.animatedContents.push({
      fn: SVGElementsRenderer.createRenderFunction(data),
      element: element,
      data: data,
    });
  };

  SVGShapeElement.prototype.setElementStyles = function(elementData) {
    const arr = elementData.styles;
    let j;
    const jLen = this.stylesList.length;
    for (j = 0; j < jLen; j += 1) {
      if (!this.stylesList[j].closed) {
        arr.push(this.stylesList[j]);
      }
    }
  };

  SVGShapeElement.prototype.reloadShapes = function() {
    this._isFirstFrame = true;
    let i;
    let len = this.itemsData.length;
    for( i = 0; i < len; i += 1) {
      this.prevViewData[i] = this.itemsData[i];
    }
    this.searchShapes(this.shapesData,this.itemsData,this.prevViewData,this.layerElement, 0, [], true);
    this.filterUniqueShapes();
    len = this.dynamicProperties.length;
    for(i = 0; i < len; i += 1) {
      this.dynamicProperties[i].getValue();
    }
    this.renderModifiers();
  };

  SVGShapeElement.prototype.searchShapes = function(
      arr, itemsData, prevViewData, container, level, transformers, render) {
    const ownTransformers = [].concat(transformers);
    let i;
    let len = arr.length - 1;
    let j;
    let jLen;
    const ownStyles = [];
    const ownModifiers = [];
    let styleOb;
    let currentTransform;
    let modifier;
    let processedPos;
    for(i=len;i>=0;i-=1){
      processedPos = this.searchProcessedElement(arr[i]);
      if (!processedPos) {
        arr[i]._render = render;
      } else {
        itemsData[i] = prevViewData[processedPos - 1];
      }
      if (arr[i].ty == 'fl' || arr[i].ty == 'st' || arr[i].ty == 'gf' ||
          arr[i].ty == 'gs') {
        if (!processedPos) {
          itemsData[i] = this.createStyleElement(arr[i], level);
        } else {
          itemsData[i].style.closed = false;
        }
        if (arr[i]._render) {
          container.appendChild(itemsData[i].style.pElem);
        }
        ownStyles.push(itemsData[i].style);
      } else if (arr[i].ty == 'gr') {
        if (!processedPos) {
          itemsData[i] = this.createGroupElement(arr[i]);
        } else {
          jLen = itemsData[i].it.length;
          for (j = 0; j < jLen; j += 1) {
            itemsData[i].prevViewData[j] = itemsData[i].it[j];
          }
        }
        this.searchShapes(
            arr[i].it, itemsData[i].it, itemsData[i].prevViewData,
            itemsData[i].gr, level + 1, ownTransformers, render);
        if (arr[i]._render) {
          container.appendChild(itemsData[i].gr);
        }
      } else if (arr[i].ty == 'tr') {
        if (!processedPos) {
          itemsData[i] = this.createTransformElement(arr[i], container);
        }
        currentTransform = itemsData[i].transform;
        ownTransformers.push(currentTransform);
      } else if (
          arr[i].ty == 'sh' || arr[i].ty == 'rc' || arr[i].ty == 'el' ||
          arr[i].ty == 'sr') {
        if (!processedPos) {
          itemsData[i] =
              this.createShapeElement(arr[i], ownTransformers, level);
        }
        this.setElementStyles(itemsData[i]);

      } else if (arr[i].ty == 'tm' || arr[i].ty == 'rd' || arr[i].ty == 'ms') {
        if (!processedPos) {
          modifier = ShapeModifiers.getModifier(arr[i].ty);
          modifier.init(this, arr[i]);
          itemsData[i] = modifier;
          this.shapeModifiers.push(modifier);
        } else {
          modifier = itemsData[i];
          modifier.closed = false;
        }
        ownModifiers.push(modifier);
      } else if (arr[i].ty == 'rp') {
        if (!processedPos) {
          modifier = ShapeModifiers.getModifier(arr[i].ty);
          itemsData[i] = modifier;
          modifier.init(this, arr, i, itemsData);
          this.shapeModifiers.push(modifier);
          render = false;
        } else {
          modifier = itemsData[i];
          modifier.closed = true;
        }
        ownModifiers.push(modifier);
      }
      this.addProcessedElement(arr[i], i + 1);
    }
    len = ownStyles.length;
    for(i=0;i<len;i+=1){
      ownStyles[i].closed = true;
    }
    len = ownModifiers.length;
    for(i=0;i<len;i+=1){
      ownModifiers[i].closed = true;
    }
  };

  SVGShapeElement.prototype.renderInnerContent = function() {
    this.renderModifiers();
    let i;
    const len = this.stylesList.length;
    for(i=0;i<len;i+=1){
      this.stylesList[i].reset();
    }
    this.renderShape();

    for (i = 0; i < len; i += 1) {
      if (this.stylesList[i]._mdf || this._isFirstFrame) {
        if (this.stylesList[i].msElem) {
          this.stylesList[i].msElem.setAttribute('d', this.stylesList[i].d);
          // Adding M0 0 fixes same mask bug on all browsers
          this.stylesList[i].d = 'M0 0' + this.stylesList[i].d;
        }
        this.stylesList[i].pElem.setAttribute(
            'd', this.stylesList[i].d || 'M0 0');
      }
    }
  };

  SVGShapeElement.prototype.renderShape = function() {
    let i;
    const len = this.animatedContents.length;
    let animatedContent;
    for(i = 0; i < len; i += 1) {
      animatedContent = this.animatedContents[i];
      if ((this._isFirstFrame || animatedContent.element._isAnimated) &&
          animatedContent.data !== true) {
        animatedContent.fn(
            animatedContent.data, animatedContent.element, this._isFirstFrame);
      }
    }
  };

  SVGShapeElement.prototype.destroy = function() {
    this.destroyBaseElement();
    this.shapesData = null;
    this.itemsData = null;
  };

  function CVContextData() {
    this.saved = [];
    this.cArrPos = 0;
    this.cTr = new Matrix();
    this.cO = 1;
    let i;
    const len = 15;
    this.savedOp = createTypedArray('float32', len);
    for(i=0;i<len;i+=1){
      this.saved[i] = createTypedArray('float32', 16);
    }
    this._length = len;
  }

  CVContextData.prototype.duplicate = function() {
    const newLength = this._length * 2;
    const currentSavedOp = this.savedOp;
    this.savedOp = createTypedArray('float32', newLength);
    this.savedOp.set(currentSavedOp);
    let i = 0;
    for(i = this._length; i < newLength; i += 1) {
      this.saved[i] = createTypedArray('float32', 16);
    }
    this._length = newLength;
  };

  CVContextData.prototype.reset = function() {
    this.cArrPos = 0;
    this.cTr.reset();
    this.cO = 1;
  };
  function CVBaseElement() {}

  CVBaseElement.prototype = {
    createElements: function() {},
    initRendererElement: function() {},
    createContainerElements: function() {
      this.canvasContext = this.globalData.canvasContext;
      this.renderableEffectsManager = new CVEffects(this);
    },
    createContent: function() {},
    setBlendMode: function() {
      const globalData = this.globalData;
      if (globalData.blendMode !== this.data.bm) {
        globalData.blendMode = this.data.bm;
        const blendModeValue = getBlendMode(this.data.bm);
        globalData.canvasContext.globalCompositeOperation = blendModeValue;
      }
    },
    createRenderableComponents: function() {
      this.maskManager = new CVMaskElement(this.data, this);
    },
    hideElement: function() {
      if (!this.hidden && (!this.isInRange || this.isTransparent)) {
        this.hidden = true;
      }
    },
    showElement: function() {
      if (this.isInRange && !this.isTransparent) {
        this.hidden = false;
        this._isFirstFrame = true;
        this.maskManager._isFirstFrame = true;
      }
    },
    renderFrame: function() {
      if (this.hidden || this.data.hd) {
        return;
      }
      this.renderTransform();
      this.renderRenderable();
      this.setBlendMode();
      this.globalData.renderer.save();
      this.globalData.renderer.ctxTransform(this.finalTransform.mat.props);
      this.globalData.renderer.ctxOpacity(this.finalTransform.mProp.o.v);
      this.renderInnerContent();
      this.globalData.renderer.restore();
      if (this.maskManager.hasMasks) {
        this.globalData.renderer.restore(true);
      }
      if (this._isFirstFrame) {
        this._isFirstFrame = false;
      }
    },
    destroy: function() {
      this.canvasContext = null;
      this.data = null;
      this.globalData = null;
      this.maskManager.destroy();
    },
    mHelper: new Matrix(),
  };
  CVBaseElement.prototype.hide = CVBaseElement.prototype.hideElement;
  CVBaseElement.prototype.show = CVBaseElement.prototype.showElement;

  function CVCompElement(data, globalData, comp) {
    this.completeLayers = false;
    this.layers = data.layers;
    this.pendingElements = [];
    this.elements = createSizedArray(this.layers.length);
    this.initElement(data, globalData, comp);
    this.tm = data.tm ? PropertyFactory.getProp(this,data.tm,0,globalData.frameRate, this) : {_placeholder:true};
  }

  extendPrototype([CanvasRenderer, ICompElement, CVBaseElement], CVCompElement);

  CVCompElement.prototype.renderInnerContent = function() {
    let i;
    const len = this.layers.length;
    for( i = len - 1; i >= 0; i -= 1 ){
      if (this.completeLayers || this.elements[i]) {
        this.elements[i].renderFrame();
      }
    }
  };

  CVCompElement.prototype.destroy = function() {
    let i;
    const len = this.layers.length;
    for( i = len - 1; i >= 0; i -= 1 ){
      if (this.elements[i]) {
        this.elements[i].destroy();
      }
    }
    this.layers = null;
    this.elements = null;
  };

  function CVMaskElement(data, element) {
    this.data = data;
    this.element = element;
    this.masksProperties = this.data.masksProperties || [];
    this.viewData = createSizedArray(this.masksProperties.length);
    let i;
    const len = this.masksProperties.length;
    let hasMasks = false;
    for (i = 0; i < len; i++) {
      if (this.masksProperties[i].mode !== 'n') {
        hasMasks = true;
      }
      this.viewData[i] = ShapePropertyFactory.getShapeProp(
          this.element, this.masksProperties[i], 3);
    }
    this.hasMasks = hasMasks;
    if(hasMasks) {
      this.element.addRenderableComponent(this);
    }
  }

  CVMaskElement.prototype.renderFrame = function() {
    if(!this.hasMasks){
      return;
    }
    const transform = this.element.finalTransform.mat;
    const ctx = this.element.canvasContext;
    let i;
    const len = this.masksProperties.length;
    let pt;
    let pts;
    let data;
    ctx.beginPath();
    for (i = 0; i < len; i++) {
      if (this.masksProperties[i].mode !== 'n') {
        if (this.masksProperties[i].inv) {
          ctx.moveTo(0, 0);
          ctx.lineTo(this.element.globalData.compSize.w, 0);
          ctx.lineTo(
              this.element.globalData.compSize.w,
              this.element.globalData.compSize.h);
          ctx.lineTo(0, this.element.globalData.compSize.h);
          ctx.lineTo(0, 0);
        }
        data = this.viewData[i].v;
        pt = transform.applyToPointArray(data.v[0][0], data.v[0][1], 0);
        ctx.moveTo(pt[0], pt[1]);
        let j;
        const jLen = data._length;
        for (j = 1; j < jLen; j++) {
          pts = transform.applyToTriplePoints(
              data.o[j - 1], data.i[j], data.v[j]);
          ctx.bezierCurveTo(pts[0], pts[1], pts[2], pts[3], pts[4], pts[5]);
        }
        pts =
            transform.applyToTriplePoints(data.o[j - 1], data.i[0], data.v[0]);
        ctx.bezierCurveTo(pts[0], pts[1], pts[2], pts[3], pts[4], pts[5]);
      }
    }
    this.element.globalData.renderer.save(true);
    ctx.clip();
  };

  CVMaskElement.prototype.getMaskProperty =
      MaskElement.prototype.getMaskProperty;

  CVMaskElement.prototype.destroy = function() {
    this.element = null;
  };
  function CVShapeElement(data, globalData, comp) {
    this.shapes = [];
    this.shapesData = data.shapes;
    this.stylesList = [];
    this.itemsData = [];
    this.prevViewData = [];
    this.shapeModifiers = [];
    this.processedElements = [];
    this.transformsManager = new ShapeTransformManager();
    this.initElement(data, globalData, comp);
  }

  extendPrototype(
      [
        BaseElement,
        TransformElement,
        CVBaseElement,
        IShapeElement,
        HierarchyElement,
        FrameElement,
        RenderableElement,
      ],
      CVShapeElement);

  CVShapeElement.prototype.initElement =
      RenderableDOMElement.prototype.initElement;

  CVShapeElement.prototype.transformHelper = {opacity: 1, _opMdf: false};

  CVShapeElement.prototype.dashResetter = [];

  CVShapeElement.prototype.createContent = function() {
    this.searchShapes(this.shapesData,this.itemsData,this.prevViewData, true, []);
  };

  CVShapeElement.prototype.createStyleElement = function(data, transforms) {
    const styleElem = {
      data: data,
      type: data.ty,
      preTransforms: this.transformsManager.addTransformSequence(transforms),
      transforms: [],
      elements: [],
      closed: data.hd === true,
    };
    const elementData = {};
    if(data.ty == 'fl' || data.ty == 'st'){
      elementData.c = PropertyFactory.getProp(this, data.c, 1, 255, this);
      if (!elementData.c.k) {
        styleElem.co = 'rgb(' + bm_floor(elementData.c.v[0]) + ',' +
            bm_floor(elementData.c.v[1]) + ',' + bm_floor(elementData.c.v[2]) +
            ')';
      }
    } else if (data.ty === 'gf' || data.ty === 'gs') {
      elementData.s = PropertyFactory.getProp(this, data.s, 1, null, this);
      elementData.e = PropertyFactory.getProp(this, data.e, 1, null, this);
      elementData.h =
          PropertyFactory.getProp(this, data.h || {k: 0}, 0, 0.01, this);
      elementData.a =
          PropertyFactory.getProp(this, data.a || {k: 0}, 0, degToRads, this);
      elementData.g = new GradientProperty(this, data.g, this);
    }
    elementData.o = PropertyFactory.getProp(this,data.o,0,0.01,this);
    if(data.ty == 'st' || data.ty == 'gs') {
      styleElem.lc = this.lcEnum[data.lc] || 'round';
      styleElem.lj = this.ljEnum[data.lj] || 'round';
      if (data.lj == 1) {
        styleElem.ml = data.ml;
      }
      elementData.w = PropertyFactory.getProp(this, data.w, 0, null, this);
      if (!elementData.w.k) {
        styleElem.wi = elementData.w.v;
      }
      if (data.d) {
        const d = new DashProperty(this, data.d, 'canvas', this);
        elementData.d = d;
        if (!elementData.d.k) {
          styleElem.da = elementData.d.dashArray;
          styleElem.do = elementData.d.dashoffset[0];
        }
      }
    } else {
      styleElem.r = data.r === 2 ? 'evenodd' : 'nonzero';
    }
    this.stylesList.push(styleElem);
    elementData.style = styleElem;
    return elementData;
  };

  CVShapeElement.prototype.createGroupElement = function(data) {
    const elementData = {
      it: [],
      prevViewData: [],
    };
    return elementData;
  };

  CVShapeElement.prototype.createTransformElement = function(data) {
    const elementData = {
      transform: {
        opacity: 1,
        _opMdf: false,
        key: this.transformsManager.getNewKey(),
        op: PropertyFactory.getProp(this, data.o, 0, 0.01, this),
        mProps: TransformPropertyFactory.getTransformProperty(this, data, this),
      },
    };
    return elementData;
  };

  CVShapeElement.prototype.createShapeElement = function(data) {
    const elementData =
        new CVShapeData(this, data, this.stylesList, this.transformsManager);

    this.shapes.push(elementData);
    this.addShapeToModifiers(elementData);
    return elementData;
  };

  CVShapeElement.prototype.reloadShapes = function() {
    this._isFirstFrame = true;
    let i;
    let len = this.itemsData.length;
    for (i = 0; i < len; i += 1) {
      this.prevViewData[i] = this.itemsData[i];
    }
    this.searchShapes(this.shapesData,this.itemsData,this.prevViewData, true, []);
    len = this.dynamicProperties.length;
    for (i = 0; i < len; i += 1) {
      this.dynamicProperties[i].getValue();
    }
    this.renderModifiers();
    this.transformsManager.processSequences(this._isFirstFrame);
  };

  CVShapeElement.prototype.addTransformToStyleList = function(transform) {
    let i;
    const len = this.stylesList.length;
    for (i = 0; i < len; i += 1) {
      if (!this.stylesList[i].closed) {
        this.stylesList[i].transforms.push(transform);
      }
    }
  };

  CVShapeElement.prototype.removeTransformFromStyleList = function() {
    let i;
    const len = this.stylesList.length;
    for (i = 0; i < len; i += 1) {
      if (!this.stylesList[i].closed) {
        this.stylesList[i].transforms.pop();
      }
    }
  };

  CVShapeElement.prototype.closeStyles = function(styles) {
    let i;
    const len = styles.length;
    let j;
    for (i = 0; i < len; i += 1) {
      styles[i].closed = true;
    }
  };

  CVShapeElement.prototype.searchShapes = function(
      arr, itemsData, prevViewData, shouldRender, transforms) {
    let i;
    let len = arr.length - 1;
    let j;
    const ownStyles = [];
    const ownModifiers = [];
    let processedPos;
    let modifier;
    let currentTransform;
    const ownTransforms = [].concat(transforms);
    for(i=len;i>=0;i-=1){
      processedPos = this.searchProcessedElement(arr[i]);
      if (!processedPos) {
        arr[i]._shouldRender = shouldRender;
      } else {
        itemsData[i] = prevViewData[processedPos - 1];
      }
      if (arr[i].ty == 'fl' || arr[i].ty == 'st' || arr[i].ty == 'gf' ||
          arr[i].ty == 'gs') {
        if (!processedPos) {
          itemsData[i] = this.createStyleElement(arr[i], ownTransforms);
        } else {
          itemsData[i].style.closed = false;
        }

        ownStyles.push(itemsData[i].style);
      } else if (arr[i].ty == 'gr') {
        if (!processedPos) {
          itemsData[i] = this.createGroupElement(arr[i]);
        } else {
          const jLen = itemsData[i].it.length;
          for (j = 0; j < jLen; j += 1) {
            itemsData[i].prevViewData[j] = itemsData[i].it[j];
          }
        }
        this.searchShapes(
            arr[i].it, itemsData[i].it, itemsData[i].prevViewData, shouldRender,
            ownTransforms);
      } else if (arr[i].ty == 'tr') {
        if (!processedPos) {
          currentTransform = this.createTransformElement(arr[i]);
          itemsData[i] = currentTransform;
        }
        ownTransforms.push(itemsData[i]);
        this.addTransformToStyleList(itemsData[i]);
      } else if (
          arr[i].ty == 'sh' || arr[i].ty == 'rc' || arr[i].ty == 'el' ||
          arr[i].ty == 'sr') {
        if (!processedPos) {
          itemsData[i] = this.createShapeElement(arr[i]);
        }

      } else if (arr[i].ty == 'tm' || arr[i].ty == 'rd') {
        if (!processedPos) {
          modifier = ShapeModifiers.getModifier(arr[i].ty);
          modifier.init(this, arr[i]);
          itemsData[i] = modifier;
          this.shapeModifiers.push(modifier);
        } else {
          modifier = itemsData[i];
          modifier.closed = false;
        }
        ownModifiers.push(modifier);
      } else if (arr[i].ty == 'rp') {
        if (!processedPos) {
          modifier = ShapeModifiers.getModifier(arr[i].ty);
          itemsData[i] = modifier;
          modifier.init(this, arr, i, itemsData);
          this.shapeModifiers.push(modifier);
          shouldRender = false;
        } else {
          modifier = itemsData[i];
          modifier.closed = true;
        }
        ownModifiers.push(modifier);
      }
      this.addProcessedElement(arr[i], i + 1);
    }
    this.removeTransformFromStyleList();
    this.closeStyles(ownStyles);
    len = ownModifiers.length;
    for(i=0;i<len;i+=1){
      ownModifiers[i].closed = true;
    }
  };

  CVShapeElement.prototype.renderInnerContent = function() {
    this.transformHelper.opacity = 1;
    this.transformHelper._opMdf = false;
    this.renderModifiers();
    this.transformsManager.processSequences(this._isFirstFrame);
    this.renderShape(this.transformHelper,this.shapesData,this.itemsData,true);
  };

  CVShapeElement.prototype.renderShapeTransform = function(
      parentTransform, groupTransform) {
    let props;
    let groupMatrix;
    if(parentTransform._opMdf || groupTransform.op._mdf || this._isFirstFrame) {
      groupTransform.opacity = parentTransform.opacity;
      groupTransform.opacity *= groupTransform.op.v;
      groupTransform._opMdf = true;
    }
  };

  CVShapeElement.prototype.drawLayer = function() {
    let i;
    const len = this.stylesList.length;
    let j;
    let k;
    let elems;
    let nodes;
    const renderer = this.globalData.renderer;
    const ctx = this.globalData.canvasContext;
    let type;
    let currentStyle;
    for(i=0;i<len;i+=1){
      currentStyle = this.stylesList[i];
      type = currentStyle.type;

      // Skipping style when
      // Stroke width equals 0
      // style should not be rendered (extra unused repeaters)
      // current opacity equals 0
      // global opacity equals 0
      if (((type === 'st' || type === 'gs') && currentStyle.wi === 0) ||
          !currentStyle.data._shouldRender || currentStyle.coOp === 0 ||
          this.globalData.currentGlobalAlpha === 0) {
        continue;
      }
      renderer.save();
      elems = currentStyle.elements;
      if (type === 'st' || type === 'gs') {
        ctx.strokeStyle = type === 'st' ? currentStyle.co : currentStyle.grd;
        ctx.lineWidth = currentStyle.wi;
        ctx.lineCap = currentStyle.lc;
        ctx.lineJoin = currentStyle.lj;
        ctx.miterLimit = currentStyle.ml || 0;
      } else {
        ctx.fillStyle = type === 'fl' ? currentStyle.co : currentStyle.grd;
      }
      renderer.ctxOpacity(currentStyle.coOp);
      if (type !== 'st' && type !== 'gs') {
        ctx.beginPath();
      }
      renderer.ctxTransform(currentStyle.preTransforms.finalTransform.props);
      const jLen = elems.length;
      for (j = 0; j < jLen; j += 1) {
        if (type === 'st' || type === 'gs') {
          ctx.beginPath();
          if (currentStyle.da) {
            ctx.setLineDash(currentStyle.da);
            ctx.lineDashOffset = currentStyle.do;
          }
        }
        nodes = elems[j].trNodes;
        const kLen = nodes.length;

        for (k = 0; k < kLen; k += 1) {
          if (nodes[k].t == 'm') {
            ctx.moveTo(nodes[k].p[0], nodes[k].p[1]);
          } else if (nodes[k].t == 'c') {
            ctx.bezierCurveTo(
                nodes[k].pts[0], nodes[k].pts[1], nodes[k].pts[2],
                nodes[k].pts[3], nodes[k].pts[4], nodes[k].pts[5]);
          } else {
            ctx.closePath();
          }
        }
        if (type === 'st' || type === 'gs') {
          ctx.stroke();
          if (currentStyle.da) {
            ctx.setLineDash(this.dashResetter);
          }
        }
      }
      if (type !== 'st' && type !== 'gs') {
        ctx.fill(currentStyle.r);
      }
      renderer.restore();
    }
  };

  CVShapeElement.prototype.renderShape = function(
      parentTransform, items, data, isMain) {
    let i;
    const len = items.length - 1;
    let groupTransform;
    groupTransform = parentTransform;
    for(i=len;i>=0;i-=1){
      if (items[i].ty == 'tr') {
        groupTransform = data[i].transform;
        this.renderShapeTransform(parentTransform, groupTransform);
      } else if (
          items[i].ty == 'sh' || items[i].ty == 'el' || items[i].ty == 'rc' ||
          items[i].ty == 'sr') {
        this.renderPath(items[i], data[i]);
      } else if (items[i].ty == 'fl') {
        this.renderFill(items[i], data[i], groupTransform);
      } else if (items[i].ty == 'st') {
        this.renderStroke(items[i], data[i], groupTransform);
      } else if (items[i].ty == 'gf' || items[i].ty == 'gs') {
        this.renderGradientFill(items[i], data[i], groupTransform);
      } else if (items[i].ty == 'gr') {
        this.renderShape(groupTransform, items[i].it, data[i].it);
      } else if (items[i].ty == 'tm') {
        //
      }
    }
    if(isMain){
      this.drawLayer();
    }
  };

  CVShapeElement.prototype.renderStyledShape = function(styledShape, shape) {
    if(this._isFirstFrame || shape._mdf || styledShape.transforms._mdf) {
      const shapeNodes = styledShape.trNodes;
      const paths = shape.paths;
      let i;
      let len;
      let j;
      const jLen = paths._length;
      shapeNodes.length = 0;
      const groupTransformMat = styledShape.transforms.finalTransform;
      for (j = 0; j < jLen; j += 1) {
        const pathNodes = paths.shapes[j];
        if (pathNodes && pathNodes.v) {
          len = pathNodes._length;
          for (i = 1; i < len; i += 1) {
            if (i === 1) {
              shapeNodes.push({
                t: 'm',
                p: groupTransformMat.applyToPointArray(
                    pathNodes.v[0][0], pathNodes.v[0][1], 0),
              });
            }
            shapeNodes.push({
              t: 'c',
              pts: groupTransformMat.applyToTriplePoints(
                  pathNodes.o[i - 1], pathNodes.i[i], pathNodes.v[i]),
            });
          }
          if (len === 1) {
            shapeNodes.push({
              t: 'm',
              p: groupTransformMat.applyToPointArray(
                  pathNodes.v[0][0], pathNodes.v[0][1], 0),
            });
          }
          if (pathNodes.c && len) {
            shapeNodes.push({
              t: 'c',
              pts: groupTransformMat.applyToTriplePoints(
                  pathNodes.o[i - 1], pathNodes.i[0], pathNodes.v[0]),
            });
            shapeNodes.push({
              t: 'z',
            });
          }
        }
      }
      styledShape.trNodes = shapeNodes;
    }
  };

  CVShapeElement.prototype.renderPath = function(pathData, itemData) {
    if(pathData.hd !== true && pathData._shouldRender) {
      let i;
      const len = itemData.styledShapes.length;
      for (i = 0; i < len; i += 1) {
        this.renderStyledShape(itemData.styledShapes[i], itemData.sh);
      }
    }
  };

  CVShapeElement.prototype.renderFill = function(
      styleData, itemData, groupTransform) {
    const styleElem = itemData.style;

    if (itemData.c._mdf || this._isFirstFrame) {
      styleElem.co = 'rgb(' + bm_floor(itemData.c.v[0]) + ',' +
          bm_floor(itemData.c.v[1]) + ',' + bm_floor(itemData.c.v[2]) + ')';
    }
    if (itemData.o._mdf || groupTransform._opMdf || this._isFirstFrame) {
      styleElem.coOp = itemData.o.v * groupTransform.opacity;
    }
  };

  CVShapeElement.prototype.renderGradientFill = function(
      styleData, itemData, groupTransform) {
    const styleElem = itemData.style;
    if(!styleElem.grd || itemData.g._mdf || itemData.s._mdf || itemData.e._mdf || (styleData.t !== 1 && (itemData.h._mdf || itemData.a._mdf))) {
      const ctx = this.globalData.canvasContext;
      let grd;
      const pt1 = itemData.s.v;
      const pt2 = itemData.e.v;
      if (styleData.t === 1) {
        grd = ctx.createLinearGradient(pt1[0], pt1[1], pt2[0], pt2[1]);
      } else {
        const rad = Math.sqrt(
            Math.pow(pt1[0] - pt2[0], 2) + Math.pow(pt1[1] - pt2[1], 2));
        const ang = Math.atan2(pt2[1] - pt1[1], pt2[0] - pt1[0]);

        const percent = itemData.h.v >= 1 ? 0.99 :
            itemData.h.v <= -1            ? -0.99 :
                                            itemData.h.v;
        const dist = rad * percent;
        const x = Math.cos(ang + itemData.a.v) * dist + pt1[0];
        const y = Math.sin(ang + itemData.a.v) * dist + pt1[1];
        grd = ctx.createRadialGradient(x, y, 0, pt1[0], pt1[1], rad);
      }

      let i;
      const len = styleData.g.p;
      const cValues = itemData.g.c;
      let opacity = 1;

      for (i = 0; i < len; i += 1) {
        if (itemData.g._hasOpacity && itemData.g._collapsable) {
          opacity = itemData.g.o[i * 2 + 1];
        }
        grd.addColorStop(
            cValues[i * 4] / 100,
            'rgba(' + cValues[i * 4 + 1] + ',' + cValues[i * 4 + 2] + ',' +
                cValues[i * 4 + 3] + ',' + opacity + ')');
      }
      styleElem.grd = grd;
    }
    styleElem.coOp = itemData.o.v*groupTransform.opacity;
  };

  CVShapeElement.prototype.renderStroke = function(
      styleData, itemData, groupTransform) {
    const styleElem = itemData.style;
    const d = itemData.d;
    if(d && (d._mdf  || this._isFirstFrame)){
      styleElem.da = d.dashArray;
      styleElem.do = d.dashoffset[0];
    }
    if(itemData.c._mdf || this._isFirstFrame){
      styleElem.co = 'rgb(' + bm_floor(itemData.c.v[0]) + ',' +
          bm_floor(itemData.c.v[1]) + ',' + bm_floor(itemData.c.v[2]) + ')';
    }
    if(itemData.o._mdf || groupTransform._opMdf || this._isFirstFrame){
      styleElem.coOp = itemData.o.v * groupTransform.opacity;
    }
    if(itemData.w._mdf || this._isFirstFrame){
      styleElem.wi = itemData.w.v;
    }
  };


  CVShapeElement.prototype.destroy = function() {
    this.shapesData = null;
    this.globalData = null;
    this.canvasContext = null;
    this.stylesList.length = 0;
    this.itemsData.length = 0;
  };


  function CVSolidElement(data, globalData, comp) {
    this.initElement(data,globalData,comp);
  }
  extendPrototype(
      [
        BaseElement,
        TransformElement,
        CVBaseElement,
        HierarchyElement,
        FrameElement,
        RenderableElement,
      ],
      CVSolidElement);

  CVSolidElement.prototype.initElement = SVGShapeElement.prototype.initElement;
  CVSolidElement.prototype.prepareFrame = IImageElement.prototype.prepareFrame;

  CVSolidElement.prototype.renderInnerContent = function() {
    const ctx = this.canvasContext;
    ctx.fillStyle = this.data.sc;
    ctx.fillRect(0, 0, this.data.sw, this.data.sh);
    //
  };
  function CVEffects() {}
  CVEffects.prototype.renderFrame = function() {};
  const animationManager =
      (function() {
        const moduleOb = {};
        const registeredAnimations = [];
        let initTime = 0;
        let len = 0;
        let playingAnimationsNum = 0;
        let _stopped = true;
        let _isFrozen = false;

        function removeElement(ev) {
          let i = 0;
          const animItem = ev.target;
          while (i < len) {
            if (registeredAnimations[i].animation === animItem) {
              registeredAnimations.splice(i, 1);
              i -= 1;
              len -= 1;
              if (!animItem.isPaused) {
                subtractPlayingCount();
              }
            }
            i += 1;
          }
        }

        function registerAnimation(element, animationData) {
          if (!element) {
            return null;
          }
          let i = 0;
          while (i < len) {
            if(registeredAnimations[i].elem == element && registeredAnimations[i].elem !== null ){
              return registeredAnimations[i].animation;
            }
            i+=1;
          }
          const animItem = new AnimationItem();
          setupAnimation(animItem, element);
          animItem.setData(element, animationData);
          return animItem;
        }

        function getRegisteredAnimations() {
          let i;
          const len = registeredAnimations.length;
          const animations = [];
          for (i = 0; i < len; i += 1) {
            animations.push(registeredAnimations[i].animation);
          }
          return animations;
        }

        function addPlayingCount() {
          playingAnimationsNum += 1;
          activate();
        }

        function subtractPlayingCount() {
          playingAnimationsNum -= 1;
        }

        function setupAnimation(animItem, element) {
          animItem.addEventListener('destroy', removeElement);
          animItem.addEventListener('_active', addPlayingCount);
          animItem.addEventListener('_idle', subtractPlayingCount);
          registeredAnimations.push({elem: element, animation: animItem});
          len += 1;
        }

        function loadAnimation(params) {
          const animItem = new AnimationItem();
          setupAnimation(animItem, null);
          animItem.setParams(params);
          return animItem;
        }


        function setSpeed(val, animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.setSpeed(val, animation);
          }
        }

        function setDirection(val, animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.setDirection(val, animation);
          }
        }

        function play(animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.play(animation);
          }
        }
        function resume(nowTime) {
          const elapsedTime = nowTime - initTime;
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.advanceTime(elapsedTime);
          }
          initTime = nowTime;
          if (playingAnimationsNum && !_isFrozen) {
            requestAnimationFrame(resume);
          } else {
            _stopped = true;
          }
        }

        function first(nowTime) {
          initTime = nowTime;
          requestAnimationFrame(resume);
        }

        function pause(animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.pause(animation);
          }
        }

        function goToAndStop(value, isFrame, animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.goToAndStop(value,isFrame,animation);
          }
        }

        function stop(animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.stop(animation);
          }
        }

        function togglePause(animation) {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.togglePause(animation);
          }
        }

        function destroy(animation) {
          let i;
          for (i = (len - 1); i >= 0; i -= 1) {
            registeredAnimations[i].animation.destroy(animation);
          }
        }

        function searchAnimations(animationData, standalone, renderer) {
          throw new Error('Cannot access DOM from worker thread');
        }

        function resize() {
          let i;
          for (i = 0; i < len; i += 1) {
            registeredAnimations[i].animation.resize();
          }
        }

        function activate() {
          if (!_isFrozen && playingAnimationsNum) {
            if(_stopped) {
              requestAnimationFrame(first);
              _stopped = false;
            }
          }
        }

        function freeze() {
          _isFrozen = true;
        }

        function unfreeze() {
          _isFrozen = false;
          activate();
        }

        moduleOb.registerAnimation = registerAnimation;
        moduleOb.loadAnimation = loadAnimation;
        moduleOb.setSpeed = setSpeed;
        moduleOb.setDirection = setDirection;
        moduleOb.play = play;
        moduleOb.pause = pause;
        moduleOb.stop = stop;
        moduleOb.togglePause = togglePause;
        moduleOb.searchAnimations = searchAnimations;
        moduleOb.resize = resize;
        // moduleOb.start = start;
        moduleOb.goToAndStop = goToAndStop;
        moduleOb.destroy = destroy;
        moduleOb.freeze = freeze;
        moduleOb.unfreeze = unfreeze;
        moduleOb.getRegisteredAnimations = getRegisteredAnimations;
        return moduleOb;
      }());

  const AnimationItem = function() {
    this._cbs = [];
    this.name = '';
    this.path = '';
    this.isLoaded = false;
    this.currentFrame = 0;
    this.currentRawFrame = 0;
    this.totalFrames = 0;
    this.frameRate = 0;
    this.frameMult = 0;
    this.playSpeed = 1;
    this.playDirection = 1;
    this.playCount = 0;
    this.animationData = {};
    this.assets = [];
    this.isPaused = true;
    this.autoplay = false;
    this.loop = true;
    this.renderer = null;
    this.animationID = createElementID();
    this.assetsPath = '';
    this.timeCompleted = 0;
    this.segmentPos = 0;
    this.subframeEnabled = subframeEnabled;
    this.segments = [];
    this._idle = true;
    this._completedLoop = false;
    this.projectInterface = ProjectInterface();
    this.imagePreloader = new ImagePreloader();
  };

  extendPrototype([BaseEvent], AnimationItem);

  AnimationItem.prototype.setParams = function(params) {
    if(params.context){
      this.context = params.context;
    }
    if(params.wrapper || params.container){
      this.wrapper = params.wrapper || params.container;
    }
    const animType = params.animType ? params.animType :
        params.renderer              ? params.renderer :
                                       'svg';
    switch(animType){
      case 'canvas':
        this.renderer = new CanvasRenderer(this, params.rendererSettings);
        break;
      case 'svg':
        this.renderer = new SVGRenderer(this, params.rendererSettings);
        break;
      default:
        this.renderer = new HybridRenderer(this, params.rendererSettings);
        break;
    }
    this.renderer.setProjectInterface(this.projectInterface);
    this.animType = animType;

    if(params.loop === '' || params.loop === null){
    }else if(params.loop === false){
      this.loop = false;
    }else if(params.loop === true){
      this.loop = true;
    }else{
      this.loop = parseInt(params.loop);
    }
    this.autoplay = 'autoplay' in params ? params.autoplay : true;
    this.name = params.name ? params.name :  '';
    this.autoloadSegments = params.hasOwnProperty('autoloadSegments') ? params.autoloadSegments :  true;
    this.assetsPath = params.assetsPath;
    if(params.animationData){
      this.configAnimation(params.animationData);
    }else if(params.path){
      if (params.path.substr(-4) != 'json') {
        if (params.path.substr(-1, 1) != '/') {
          params.path += '/';
        }
        params.path += 'data.json';
      }

      if (params.path.lastIndexOf('\\') != -1) {
        this.path = params.path.substr(0, params.path.lastIndexOf('\\') + 1);
      } else {
        this.path = params.path.substr(0, params.path.lastIndexOf('/') + 1);
      }
      this.fileName = params.path.substr(params.path.lastIndexOf('/') + 1);
      this.fileName =
          this.fileName.substr(0, this.fileName.lastIndexOf('.json'));

      assetLoader.load(
          params.path, this.configAnimation.bind(this), function() {
            this.trigger('data_failed');
          }.bind(this));
    }
  };

  AnimationItem.prototype.setData = function(wrapper, animationData) {
    const params = {
      wrapper: wrapper,
      animationData: animationData ? (typeof animationData === 'object') ?
                                     animationData :
                                     JSON.parse(animationData) :
                                     null,
    };
    const wrapperAttributes = wrapper.attributes;

    params.path = wrapperAttributes.getNamedItem('data-animation-path') ? wrapperAttributes.getNamedItem('data-animation-path').value : wrapperAttributes.getNamedItem('data-bm-path') ? wrapperAttributes.getNamedItem('data-bm-path').value :  wrapperAttributes.getNamedItem('bm-path') ? wrapperAttributes.getNamedItem('bm-path').value : '';
    params.animType = wrapperAttributes.getNamedItem('data-anim-type') ? wrapperAttributes.getNamedItem('data-anim-type').value : wrapperAttributes.getNamedItem('data-bm-type') ? wrapperAttributes.getNamedItem('data-bm-type').value : wrapperAttributes.getNamedItem('bm-type') ? wrapperAttributes.getNamedItem('bm-type').value :  wrapperAttributes.getNamedItem('data-bm-renderer') ? wrapperAttributes.getNamedItem('data-bm-renderer').value : wrapperAttributes.getNamedItem('bm-renderer') ? wrapperAttributes.getNamedItem('bm-renderer').value : 'canvas';

    const loop = wrapperAttributes.getNamedItem('data-anim-loop') ?
        wrapperAttributes.getNamedItem('data-anim-loop').value :
        wrapperAttributes.getNamedItem('data-bm-loop') ?
        wrapperAttributes.getNamedItem('data-bm-loop').value :
        wrapperAttributes.getNamedItem('bm-loop') ?
        wrapperAttributes.getNamedItem('bm-loop').value :
        '';
    if(loop === ''){
    }else if(loop === 'false'){
      params.loop = false;
    }else if(loop === 'true'){
      params.loop = true;
    }else{
      params.loop = parseInt(loop);
    }
    const autoplay = wrapperAttributes.getNamedItem('data-anim-autoplay') ?
        wrapperAttributes.getNamedItem('data-anim-autoplay').value :
        wrapperAttributes.getNamedItem('data-bm-autoplay') ?
        wrapperAttributes.getNamedItem('data-bm-autoplay').value :
        wrapperAttributes.getNamedItem('bm-autoplay') ?
        wrapperAttributes.getNamedItem('bm-autoplay').value :
        true;
    params.autoplay = autoplay !== 'false';

    params.name = wrapperAttributes.getNamedItem('data-name') ? wrapperAttributes.getNamedItem('data-name').value :  wrapperAttributes.getNamedItem('data-bm-name') ? wrapperAttributes.getNamedItem('data-bm-name').value : wrapperAttributes.getNamedItem('bm-name') ? wrapperAttributes.getNamedItem('bm-name').value :  '';
    const prerender = wrapperAttributes.getNamedItem('data-anim-prerender') ?
        wrapperAttributes.getNamedItem('data-anim-prerender').value :
        wrapperAttributes.getNamedItem('data-bm-prerender') ?
        wrapperAttributes.getNamedItem('data-bm-prerender').value :
        wrapperAttributes.getNamedItem('bm-prerender') ?
        wrapperAttributes.getNamedItem('bm-prerender').value :
        '';

    if(prerender === 'false'){
      params.prerender = false;
    }
    this.setParams(params);
  };

  AnimationItem.prototype.includeLayers = function(data) {
    if(data.op > this.animationData.op){
      this.animationData.op = data.op;
      this.totalFrames = Math.floor(data.op - this.animationData.ip);
    }
    const layers = this.animationData.layers;
    let i;
    let len = layers.length;
    const newLayers = data.layers;
    let j;
    const jLen = newLayers.length;
    for(j=0;j<jLen;j+=1){
      i = 0;
      while (i < len) {
        if (layers[i].id == newLayers[j].id) {
          layers[i] = newLayers[j];
          break;
        }
        i += 1;
      }
    }
    if(data.chars || data.fonts){
      this.renderer.globalData.fontManager.addChars(data.chars);
      this.renderer.globalData.fontManager.addFonts(
          data.fonts, this.renderer.globalData.defs);
    }
    if(data.assets){
      len = data.assets.length;
      for (i = 0; i < len; i += 1) {
        this.animationData.assets.push(data.assets[i]);
      }
    }
    this.animationData.__complete = false;
    dataManager.completeData(this.animationData,this.renderer.globalData.fontManager);
    this.renderer.includeLayers(data.layers);
    if(expressionsPlugin){
      expressionsPlugin.initExpressions(this);
    }
    this.loadNextSegment();
  };

  AnimationItem.prototype.loadNextSegment = function() {
    const segments = this.animationData.segments;
    if(!segments || segments.length === 0 || !this.autoloadSegments){
      this.trigger('data_ready');
      this.timeCompleted = this.totalFrames;
      return;
    }
    const segment = segments.shift();
    this.timeCompleted = segment.time * this.frameRate;
    const segmentPath =
        this.path + this.fileName + '_' + this.segmentPos + '.json';
    this.segmentPos += 1;
    assetLoader.load(segmentPath, this.includeLayers.bind(this), function() {
        this.trigger('data_failed');
    }.bind(this));
  };

  AnimationItem.prototype.loadSegments = function() {
    const segments = this.animationData.segments;
    if(!segments) {
      this.timeCompleted = this.totalFrames;
    }
    this.loadNextSegment();
  };

  AnimationItem.prototype.imagesLoaded = function() {
    this.trigger('loaded_images');
    this.checkLoaded();
  };

  AnimationItem.prototype.preloadImages = function() {
    this.imagePreloader.setAssetsPath(this.assetsPath);
    this.imagePreloader.setPath(this.path);
    this.imagePreloader.loadAssets(this.animationData.assets, this.imagesLoaded.bind(this));
  };

  AnimationItem.prototype.configAnimation = function(animData) {
    if(!this.renderer){
      return;
    }
    this.animationData = animData;
    this.totalFrames = Math.floor(this.animationData.op - this.animationData.ip);
    this.renderer.configAnimation(animData);
    if(!animData.assets){
      animData.assets = [];
    }
    this.renderer.searchExtraCompositions(animData.assets);

    this.assets = this.animationData.assets;
    this.frameRate = this.animationData.fr;
    this.firstFrame = Math.round(this.animationData.ip);
    this.frameMult = this.animationData.fr / 1000;
    this.trigger('config_ready');
    this.preloadImages();
    this.loadSegments();
    this.updaFrameModifier();
    this.waitForFontsLoaded();
  };

  AnimationItem.prototype.waitForFontsLoaded = function() {
    if(!this.renderer) {
      return;
    }
    if(this.renderer.globalData.fontManager.loaded()){
      this.checkLoaded();
    }else{
      setTimeout(this.waitForFontsLoaded.bind(this), 20);
    }
  };

  AnimationItem.prototype.checkLoaded = function() {
    if (!this.isLoaded && this.renderer.globalData.fontManager.loaded() && (this.imagePreloader.loaded() || this.renderer.rendererType !== 'canvas')) {
      this.isLoaded = true;
      dataManager.completeData(
          this.animationData, this.renderer.globalData.fontManager);
      if (expressionsPlugin) {
        expressionsPlugin.initExpressions(this);
      }
      this.renderer.initItems();
      setTimeout(function() {
        this.trigger('DOMLoaded');
      }.bind(this), 0);
      this.gotoFrame();
      if (this.autoplay) {
        this.play();
      }
    }
  };

  AnimationItem.prototype.resize = function() {
    this.renderer.updateContainerSize();
  };

  AnimationItem.prototype.setSubframe = function(flag) {
    this.subframeEnabled = flag ? true : false;
  };

  AnimationItem.prototype.gotoFrame = function() {
    this.currentFrame = this.subframeEnabled ? this.currentRawFrame : ~~this.currentRawFrame;

    if(this.timeCompleted !== this.totalFrames && this.currentFrame > this.timeCompleted){
      this.currentFrame = this.timeCompleted;
    }
    this.trigger('enterFrame');
    this.renderFrame();
  };

  AnimationItem.prototype.renderFrame = function() {
    if(this.isLoaded === false){
      return;
    }
    this.renderer.renderFrame(this.currentFrame + this.firstFrame);
  };

  AnimationItem.prototype.play = function(name) {
    if(name && this.name != name){
      return;
    }
    if(this.isPaused === true){
      this.isPaused = false;
      if (this._idle) {
        this._idle = false;
        this.trigger('_active');
      }
    }
  };

  AnimationItem.prototype.pause = function(name) {
    if(name && this.name != name){
      return;
    }
    if(this.isPaused === false){
      this.isPaused = true;
      this._idle = true;
      this.trigger('_idle');
    }
  };

  AnimationItem.prototype.togglePause = function(name) {
    if(name && this.name != name){
      return;
    }
    if(this.isPaused === true){
      this.play();
    }else{
      this.pause();
    }
  };

  AnimationItem.prototype.stop = function(name) {
    if(name && this.name != name){
      return;
    }
    this.pause();
    this.playCount = 0;
    this._completedLoop = false;
    this.setCurrentRawFrameValue(0);
  };

  AnimationItem.prototype.goToAndStop = function(value, isFrame, name) {
    if(name && this.name != name){
      return;
    }
    if(isFrame){
      this.setCurrentRawFrameValue(value);
    }else{
      this.setCurrentRawFrameValue(value * this.frameModifier);
    }
    this.pause();
  };

  AnimationItem.prototype.goToAndPlay = function(value, isFrame, name) {
    this.goToAndStop(value, isFrame, name);
    this.play();
  };

  AnimationItem.prototype.advanceTime = function(value) {
    if (this.isPaused === true || this.isLoaded === false) {
      return;
    }
    let nextValue = this.currentRawFrame + value * this.frameModifier;
    let _isComplete = false;
    // Checking if nextValue > totalFrames - 1 for addressing non looping and looping animations.
    // If animation won't loop, it should stop at totalFrames - 1. If it will loop it should complete the last frame and then loop.
    if (nextValue >= this.totalFrames - 1 && this.frameModifier > 0) {
      if (!this.loop || this.playCount === this.loop) {
        if (!this.checkSegments(
                nextValue > this.totalFrames ? nextValue % this.totalFrames :
                                               0)) {
          _isComplete = true;
          nextValue = this.totalFrames - 1;
        }
      } else if (nextValue >= this.totalFrames) {
        this.playCount += 1;
        if (!this.checkSegments(nextValue % this.totalFrames)) {
          this.setCurrentRawFrameValue(nextValue % this.totalFrames);
          this._completedLoop = true;
          this.trigger('loopComplete');
        }
      } else {
        this.setCurrentRawFrameValue(nextValue);
      }
    } else if(nextValue < 0) {
      if (!this.checkSegments(nextValue % this.totalFrames)) {
        if (this.loop && !(this.playCount-- <= 0 && this.loop !== true)) {
          this.setCurrentRawFrameValue(
              this.totalFrames + (nextValue % this.totalFrames));
          if (!this._completedLoop) {
            this._completedLoop = true;
          } else {
            this.trigger('loopComplete');
          }
        } else {
          _isComplete = true;
          nextValue = 0;
        }
      }
    } else {
      this.setCurrentRawFrameValue(nextValue);
    }
    if (_isComplete) {
      this.setCurrentRawFrameValue(nextValue);
      this.pause();
      this.trigger('complete');
    }
  };

  AnimationItem.prototype.adjustSegment = function(arr, offset) {
    this.playCount = 0;
    if(arr[1] < arr[0]){
      if (this.frameModifier > 0) {
        if (this.playSpeed < 0) {
          this.setSpeed(-this.playSpeed);
        } else {
          this.setDirection(-1);
        }
      }
      this.timeCompleted = this.totalFrames = arr[0] - arr[1];
      this.firstFrame = arr[1];
      this.setCurrentRawFrameValue(this.totalFrames - 0.001 - offset);
    } else if(arr[1] > arr[0]){
      if (this.frameModifier < 0) {
        if (this.playSpeed < 0) {
          this.setSpeed(-this.playSpeed);
        } else {
          this.setDirection(1);
        }
      }
      this.timeCompleted = this.totalFrames = arr[1] - arr[0];
      this.firstFrame = arr[0];
      this.setCurrentRawFrameValue(0.001 + offset);
    }
    this.trigger('segmentStart');
  };
  AnimationItem.prototype.setSegment = function(init, end) {
    let pendingFrame = -1;
    if(this.isPaused) {
      if (this.currentRawFrame + this.firstFrame < init) {
        pendingFrame = init;
      } else if (this.currentRawFrame + this.firstFrame > end) {
        pendingFrame = end - init;
      }
    }

    this.firstFrame = init;
    this.timeCompleted = this.totalFrames = end - init;
    if(pendingFrame !== -1) {
      this.goToAndStop(pendingFrame, true);
    }
  };

  AnimationItem.prototype.playSegments = function(arr, forceFlag) {
    if (forceFlag) {
      this.segments.length = 0;
    }
    if (typeof arr[0] === 'object') {
      let i;
      const len = arr.length;
      for (i = 0; i < len; i += 1) {
        this.segments.push(arr[i]);
      }
    } else {
      this.segments.push(arr);
    }
    if (this.segments.length && forceFlag) {
      this.adjustSegment(this.segments.shift(), 0);
    }
    if (this.isPaused) {
      this.play();
    }
  };

  AnimationItem.prototype.resetSegments = function(forceFlag) {
    this.segments.length = 0;
    this.segments.push([this.animationData.ip,this.animationData.op]);
    //this.segments.push([this.animationData.ip*this.frameRate,Math.floor(this.animationData.op - this.animationData.ip+this.animationData.ip*this.frameRate)]);
    if (forceFlag) {
      this.checkSegments(0);
    }
  };
  AnimationItem.prototype.checkSegments = function(offset) {
    if (this.segments.length) {
      this.adjustSegment(this.segments.shift(), offset);
      return true;
    }
    return false;
  };

  AnimationItem.prototype.destroy = function(name) {
    if ((name && this.name != name) || !this.renderer) {
      return;
    }
    this.renderer.destroy();
    this.imagePreloader.destroy();
    this.trigger('destroy');
    this._cbs = null;
    this.onEnterFrame = this.onLoopComplete = this.onComplete = this.onSegmentStart = this.onDestroy = null;
    this.renderer = null;
  };

  AnimationItem.prototype.setCurrentRawFrameValue = function(value) {
    this.currentRawFrame = value;
    this.gotoFrame();
  };

  AnimationItem.prototype.setSpeed = function(val) {
    this.playSpeed = val;
    this.updaFrameModifier();
  };

  AnimationItem.prototype.setDirection = function(val) {
    this.playDirection = val < 0 ? -1 : 1;
    this.updaFrameModifier();
  };

  AnimationItem.prototype.updaFrameModifier = function() {
    this.frameModifier = this.frameMult * this.playSpeed * this.playDirection;
  };

  AnimationItem.prototype.getPath = function() {
    return this.path;
  };

  AnimationItem.prototype.getAssetsPath = function(assetData) {
    let path = '';
    if(assetData.e) {
      path = assetData.p;
    } else if(this.assetsPath){
      let imagePath = assetData.p;
      if (imagePath.indexOf('images/') !== -1) {
        imagePath = imagePath.split('/')[1];
      }
      path = this.assetsPath + imagePath;
    } else {
      path = this.path;
      path += assetData.u ? assetData.u : '';
      path += assetData.p;
    }
    return path;
  };

  AnimationItem.prototype.getAssetData = function(id) {
    let i = 0;
    const len = this.assets.length;
    while (i < len) {
      if (id == this.assets[i].id) {
        return this.assets[i];
      }
      i += 1;
    }
  };

  AnimationItem.prototype.hide = function() {
    this.renderer.hide();
  };

  AnimationItem.prototype.show = function() {
    this.renderer.show();
  };

  AnimationItem.prototype.getDuration = function(isFrame) {
    return isFrame ? this.totalFrames : this.totalFrames / this.frameRate;
  };

  AnimationItem.prototype.trigger = function(name) {
    if(this._cbs && this._cbs[name]){
      switch (name) {
        case 'enterFrame':
          this.triggerEvent(
              name,
              new BMEnterFrameEvent(
                  name, this.currentFrame, this.totalFrames,
                  this.frameModifier));
          break;
        case 'loopComplete':
          this.triggerEvent(
              name,
              new BMCompleteLoopEvent(
                  name, this.loop, this.playCount, this.frameMult));
          break;
        case 'complete':
          this.triggerEvent(name, new BMCompleteEvent(name, this.frameMult));
          break;
        case 'segmentStart':
          this.triggerEvent(
              name,
              new BMSegmentStartEvent(name, this.firstFrame, this.totalFrames));
          break;
        case 'destroy':
          this.triggerEvent(name, new BMDestroyEvent(name, this));
          break;
        default:
          this.triggerEvent(name);
      }
    }
    if(name === 'enterFrame' && this.onEnterFrame){
      this.onEnterFrame.call(
          this,
          new BMEnterFrameEvent(
              name, this.currentFrame, this.totalFrames, this.frameMult));
    }
    if(name === 'loopComplete' && this.onLoopComplete){
      this.onLoopComplete.call(
          this,
          new BMCompleteLoopEvent(
              name, this.loop, this.playCount, this.frameMult));
    }
    if(name === 'complete' && this.onComplete){
      this.onComplete.call(this, new BMCompleteEvent(name, this.frameMult));
    }
    if(name === 'segmentStart' && this.onSegmentStart){
      this.onSegmentStart.call(
          this,
          new BMSegmentStartEvent(name, this.firstFrame, this.totalFrames));
    }
    if(name === 'destroy' && this.onDestroy){
      this.onDestroy.call(this, new BMDestroyEvent(name, this));
    }
  };

  AnimationItem.prototype.setParams = function(params) {
    if(params.context){
      this.context = params.context;
    }
    const animType = params.animType ? params.animType :
        params.renderer              ? params.renderer :
                                       'svg';
    switch(animType){
      case 'canvas':
        this.renderer = new CanvasRenderer(this, params.rendererSettings);
        break;
      default:
        throw new Error('Only canvas renderer is supported when using worker.');
        break;
    }
    this.renderer.setProjectInterface(this.projectInterface);
    this.animType = animType;

    if(params.loop === '' || params.loop === null){
    }else if(params.loop === false){
      this.loop = false;
    }else if(params.loop === true){
      this.loop = true;
    }else{
      this.loop = parseInt(params.loop);
    }
    this.autoplay = 'autoplay' in params ? params.autoplay : true;
    this.name = params.name ? params.name :  '';
    this.autoloadSegments = params.hasOwnProperty('autoloadSegments') ? params.autoloadSegments :  true;
    this.assetsPath = null;
    if(params.animationData){
      this.configAnimation(params.animationData);
    }else if(params.path){
      throw new Error('Canvas worker renderer cannot load animation from url');
    }
  };

  AnimationItem.prototype.setData = function(wrapper, animationData) {
    throw new Error('Cannot set data on wrapper for canvas worker renderer');
  };

  AnimationItem.prototype.includeLayers = function(data) {
    if(data.op > this.animationData.op){
      this.animationData.op = data.op;
      this.totalFrames = Math.floor(data.op - this.animationData.ip);
    }
    const layers = this.animationData.layers;
    let i;
    const len = layers.length;
    const newLayers = data.layers;
    let j;
    const jLen = newLayers.length;
    for(j=0;j<jLen;j+=1){
      i = 0;
      while (i < len) {
        if (layers[i].id == newLayers[j].id) {
          layers[i] = newLayers[j];
          break;
        }
        i += 1;
      }
    }
    this.animationData.__complete = false;
    dataManager.completeData(this.animationData,this.renderer.globalData.fontManager);
    this.renderer.includeLayers(data.layers);
    if(expressionsPlugin){
      expressionsPlugin.initExpressions(this);
    }
    this.loadNextSegment();
  };

  AnimationItem.prototype.loadNextSegment = function() {
    const segments = this.animationData.segments;
    if(!segments || segments.length === 0 || !this.autoloadSegments){
      this.timeCompleted = this.totalFrames;
      return;
    }
    throw new Error('Cannot load multiple segments in worker.');
  };

  AnimationItem.prototype.loadSegments = function() {
    const segments = this.animationData.segments;
    if(!segments) {
      this.timeCompleted = this.totalFrames;
    }
    this.loadNextSegment();
  };

  AnimationItem.prototype.imagesLoaded = null;

  AnimationItem.prototype.preloadImages = null;

  AnimationItem.prototype.configAnimation = function(animData) {
    if(!this.renderer){
      return;
    }
    this.animationData = animData;
    this.totalFrames = Math.floor(this.animationData.op - this.animationData.ip);
    this.renderer.configAnimation(animData);
    if(!animData.assets){
      animData.assets = [];
    }
    this.renderer.searchExtraCompositions(animData.assets);

    this.assets = this.animationData.assets;
    this.frameRate = this.animationData.fr;
    this.firstFrame = Math.round(this.animationData.ip);
    this.frameMult = this.animationData.fr / 1000;
    this.loadSegments();
    this.updaFrameModifier();
    this.checkLoaded();
  };

  AnimationItem.prototype.waitForFontsLoaded = null;

  AnimationItem.prototype.checkLoaded = function() {
    if (!this.isLoaded) {
      this.isLoaded = true;
      dataManager.completeData(this.animationData, null);
      if (expressionsPlugin) {
        expressionsPlugin.initExpressions(this);
      }
      this.renderer.initItems();
      this.gotoFrame();
    }
  };

  AnimationItem.prototype.destroy = function(name) {
    if ((name && this.name != name) || !this.renderer) {
      return;
    }
    this.renderer.destroy();
    this._cbs = null;
    this.onEnterFrame = this.onLoopComplete = this.onComplete = this.onSegmentStart = this.onDestroy = null;
    this.renderer = null;
  };

  AnimationItem.prototype.getPath = null;

  const Expressions = (function() {
    const ob = {};
    ob.initExpressions = initExpressions;


    function initExpressions(animation){
      let stackCount = 0;
      const registers = [];

      function pushExpression() {
        stackCount += 1;
      }

      function popExpression() {
        stackCount -= 1;
        if (stackCount === 0) {
          releaseInstances();
        }
      }

      function registerExpressionProperty(expression) {
        if (registers.indexOf(expression) === -1) {
          registers.push(expression);
        }
      }

      function releaseInstances() {
        let i;
        const len = registers.length;
        for (i = 0; i < len; i += 1) {
          registers[i].release();
        }
        registers.length = 0;
      }

      animation.renderer.compInterface =
          CompExpressionInterface(animation.renderer);
      animation.renderer.globalData.projectInterface.registerComposition(
          animation.renderer);
      animation.renderer.globalData.pushExpression = pushExpression;
      animation.renderer.globalData.popExpression = popExpression;
      animation.renderer.globalData.registerExpressionProperty =
          registerExpressionProperty;
    }
    return ob;
  }());

  expressionsPlugin = Expressions;

  const ExpressionManager =
      (function() {
        'use strict';
        const ob = {};
        const Math = BMMath;
        const window = null;
        const document = null;

        function $bm_isInstanceOfArray(arr) {
          return arr.constructor === Array || arr.constructor === Float32Array;
        }

        function isNumerable(tOfV, v) {
          return tOfV === 'number' || tOfV === 'boolean' || tOfV === 'string' ||
              v instanceof Number;
        }

        function $bm_neg(a) {
          const tOfA = typeof a;
          if (tOfA === 'number' || tOfA === 'boolean' || a instanceof Number) {
            return -a;
          }
          if ($bm_isInstanceOfArray(a)) {
            let i;
            const lenA = a.length;
            const retArr = [];
            for(i=0;i<lenA;i+=1){
              retArr[i] = -a[i];
            }
            return retArr;
          }
          if (a.propType) {
            return a.v;
          }
        }

        const easeInBez =
            BezierFactory.getBezierEasing(0.333, 0, .833, .833, 'easeIn').get;
        const easeOutBez =
            BezierFactory.getBezierEasing(0.167, 0.167, .667, 1, 'easeOut').get;
        const easeInOutBez =
            BezierFactory.getBezierEasing(.33, 0, .667, 1, 'easeInOut').get;

        function sum(a, b) {
          const tOfA = typeof a;
          const tOfB = typeof b;
          if (tOfA === 'string' || tOfB === 'string') {
            return a + b;
          }
          if (isNumerable(tOfA, a) && isNumerable(tOfB, b)) {
            return a + b;
          }
          if ($bm_isInstanceOfArray(a) && isNumerable(tOfB, b)) {
            a = a.slice(0);
            a[0] = a[0] + b;
            return a;
          }
          if (isNumerable(tOfA, a) && $bm_isInstanceOfArray(b)) {
            b = b.slice(0);
            b[0] = a + b[0];
            return b;
          }
          if ($bm_isInstanceOfArray(a) && $bm_isInstanceOfArray(b)) {
            let i = 0;
            const lenA = a.length;
            const lenB = b.length;
            const retArr = [];
            while(i<lenA || i < lenB){
              if ((typeof a[i] === 'number' || a[i] instanceof Number) &&
                  (typeof b[i] === 'number' || b[i] instanceof Number)) {
                retArr[i] = a[i] + b[i];
              } else {
                retArr[i] = b[i] === undefined ? a[i] : a[i] || b[i];
              }
              i += 1;
            }
            return retArr;
          }
          return 0;
        }
        const add = sum;

        function sub(a, b) {
          const tOfA = typeof a;
          const tOfB = typeof b;
          if (isNumerable(tOfA, a) && isNumerable(tOfB, b)) {
            if(tOfA === 'string') {
              a = parseInt(a);
            }
            if(tOfB === 'string') {
              b = parseInt(b);
            }
            return a - b;
          }
          if ($bm_isInstanceOfArray(a) && isNumerable(tOfB, b)) {
            a = a.slice(0);
            a[0] = a[0] - b;
            return a;
          }
          if (isNumerable(tOfA, a) && $bm_isInstanceOfArray(b)) {
            b = b.slice(0);
            b[0] = a - b[0];
            return b;
          }
          if ($bm_isInstanceOfArray(a) && $bm_isInstanceOfArray(b)) {
            let i = 0;
            const lenA = a.length;
            const lenB = b.length;
            const retArr = [];
            while(i<lenA || i < lenB){
              if ((typeof a[i] === 'number' || a[i] instanceof Number) &&
                  (typeof b[i] === 'number' || b[i] instanceof Number)) {
                retArr[i] = a[i] - b[i];
              } else {
                retArr[i] = b[i] === undefined ? a[i] : a[i] || b[i];
              }
              i += 1;
            }
            return retArr;
          }
          return 0;
        }

        function mul(a, b) {
          const tOfA = typeof a;
          const tOfB = typeof b;
          let arr;
          if (isNumerable(tOfA, a) && isNumerable(tOfB, b)) {
            return a * b;
          }

          let i;
          let len;
          if ($bm_isInstanceOfArray(a) && isNumerable(tOfB, b)) {
            len = a.length;
            arr = createTypedArray('float32', len);
            for(i=0;i<len;i+=1){
              arr[i] = a[i] * b;
            }
            return arr;
          }
          if (isNumerable(tOfA, a) && $bm_isInstanceOfArray(b)) {
            len = b.length;
            arr = createTypedArray('float32', len);
            for(i=0;i<len;i+=1){
              arr[i] = a * b[i];
            }
            return arr;
          }
          return 0;
        }

        function div(a, b) {
          const tOfA = typeof a;
          const tOfB = typeof b;
          let arr;
          if (isNumerable(tOfA, a) && isNumerable(tOfB, b)) {
            return a / b;
          }
          let i;
          let len;
          if ($bm_isInstanceOfArray(a) && isNumerable(tOfB, b)) {
            len = a.length;
            arr = createTypedArray('float32', len);
            for(i=0;i<len;i+=1){
              arr[i] = a[i] / b;
            }
            return arr;
          }
          if (isNumerable(tOfA, a) && $bm_isInstanceOfArray(b)) {
            len = b.length;
            arr = createTypedArray('float32', len);
            for(i=0;i<len;i+=1){
              arr[i] = a / b[i];
            }
            return arr;
          }
          return 0;
        }
        function mod(a, b) {
          if (typeof a === 'string') {
            a = parseInt(a);
          }
          if (typeof b === 'string') {
            b = parseInt(b);
          }
          return a % b;
        }
        const $bm_sum = sum;
        const $bm_sub = sub;
        const $bm_mul = mul;
        const $bm_div = div;
        const $bm_mod = mod;

        function clamp(num, min, max) {
          if (min > max) {
            const mm = max;
            max = min;
            min = mm;
          }
          return Math.min(Math.max(num, min), max);
        }

        function radiansToDegrees(val) {
          return val / degToRads;
        }
        const radians_to_degrees = radiansToDegrees;

        function degreesToRadians(val) {
          return val * degToRads;
        }
        const degrees_to_radians = radiansToDegrees;

        const helperLengthArray = [0, 0, 0, 0, 0, 0];

        function length(arr1, arr2) {
          if (typeof arr1 === 'number' || arr1 instanceof Number) {
            arr2 = arr2 || 0;
            return Math.abs(arr1 - arr2);
          }
          if (!arr2) {
            arr2 = helperLengthArray;
          }
          let i;
          const len = Math.min(arr1.length, arr2.length);
          let addedLength = 0;
          for (i = 0; i < len; i += 1) {
            addedLength += Math.pow(arr2[i] - arr1[i], 2);
          }
          return Math.sqrt(addedLength);
        }

        function normalize(vec) {
          return div(vec, length(vec));
        }

        function rgbToHsl(val) {
          const r = val[0];
          const g = val[1];
          const b = val[2];
          const max = Math.max(r, g, b);
          const min = Math.min(r, g, b);
          let h;
          let s;
          const l = (max + min) / 2;

          if (max == min) {
            h = s = 0; // achromatic
          } else {
            const d = max - min;
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
            switch(max){
              case r:
                h = (g - b) / d + (g < b ? 6 : 0);
                break;
              case g:
                h = (b - r) / d + 2;
                break;
              case b:
                h = (r - g) / d + 4;
                break;
            }
            h /= 6;
          }

          return [h, s, l, val[3]];
        }

        function hue2rgb(p, q, t) {
          if (t < 0) {
            t += 1;
          }
          if (t > 1) {
            t -= 1;
          }
          if (t < 1 / 6) {
            return p + (q - p) * 6 * t;
          }
          if (t < 1 / 2) {
            return q;
          }
          if (t < 2 / 3) {
            return p + (q - p) * (2 / 3 - t) * 6;
          }
          return p;
        }

        function hslToRgb(val) {
          const h = val[0];
          const s = val[1];
          const l = val[2];

          let r;
          let g;
          let b;

          if (s === 0) {
            r = g = b = l; // achromatic
          } else {
            const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            const p = 2 * l - q;
            r = hue2rgb(p, q, h + 1/3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1/3);
          }

          return [r, g, b, val[3]];
        }

        function linear(t, tMin, tMax, value1, value2) {
          if (value1 === undefined || value2 === undefined) {
            value1 = tMin;
            value2 = tMax;
            tMin = 0;
            tMax = 1;
          }
          if (tMax < tMin) {
            const _tMin = tMax;
            tMax = tMin;
            tMin = _tMin;
          }
          if (t <= tMin) {
            return value1;
          } else if (t >= tMax) {
            return value2;
          }
          const perc = tMax === tMin ? 0 : (t - tMin) / (tMax - tMin);
          if (!value1.length) {
            return value1 + (value2-value1)*perc;
          }
          let i;
          const len = value1.length;
          const arr = createTypedArray('float32', len);
          for (i = 0; i < len; i += 1) {
            arr[i] = value1[i] + (value2[i]-value1[i])*perc;
          }
          return arr;
        }
        function random(min, max) {
          if (max === undefined) {
            if(min === undefined){
              min = 0;
              max = 1;
            } else {
              max = min;
              min = undefined;
            }
          }
          if (max.length) {
            let i;
            const len = max.length;
            if(!min){
              min = createTypedArray('float32', len);
            }
            const arr = createTypedArray('float32', len);
            const rnd = BMMath.random();
            for(i=0;i<len;i+=1){
              arr[i] = min[i] + rnd * (max[i] - min[i]);
            }
            return arr;
          }
          if (min === undefined) {
            min = 0;
          }
          const rndm = BMMath.random();
          return min + rndm * (max - min);
        }

        function createPath(points, inTangents, outTangents, closed) {
          let i;
          const len = points.length;
          const path = shape_pool.newElement();
          path.setPathData(!!closed, len);
          const arrPlaceholder = [0, 0];
          let inVertexPoint;
          let outVertexPoint;
          for (i = 0; i < len; i += 1) {
            inVertexPoint = (inTangents && inTangents[i]) ? inTangents[i] : arrPlaceholder;
            outVertexPoint = (outTangents && outTangents[i]) ? outTangents[i] : arrPlaceholder;
            path.setTripleAt(points[i][0],points[i][1],outVertexPoint[0] + points[i][0],outVertexPoint[1] + points[i][1],inVertexPoint[0] + points[i][0],inVertexPoint[1] + points[i][1],i,true);
          }
          return path;
        }

        function initiateExpression(elem, data, property) {
          const val = data.x;
          const needsVelocity = /velocity(?![\w\d])/.test(val);
          const _needsRandom = val.indexOf('random') !== -1;
          const elemType = elem.data.ty;
          let transform;
          let $bm_transform;
          let content;
          let effect;
          const thisProperty = property;
          thisProperty.valueAtTime = thisProperty.getValueAtTime;
          Object.defineProperty(thisProperty, 'value', {
            get: function() {
              return thisProperty.v;
            },
          });
          elem.comp.frameDuration = 1 / elem.comp.globalData.frameRate;
          elem.comp.displayStartTime = 0;
          const inPoint = elem.data.ip / elem.comp.globalData.frameRate;
          const outPoint = elem.data.op / elem.comp.globalData.frameRate;
          const width = elem.data.sw ? elem.data.sw : 0;
          const height = elem.data.sh ? elem.data.sh : 0;
          const name = elem.data.nm;
          let loopIn;
          let loop_in;
          let loopOut;
          let loop_out;
          let smooth;
          let toWorld;
          let fromWorld;
          let fromComp;
          let toComp;
          let fromCompToSurface;
          let position;
          let rotation;
          let anchorPoint;
          let scale;
          let thisLayer;
          let thisComp;
          let mask;
          let valueAtTime;
          let velocityAtTime;
          const __expression_functions = [];
          if (data.xf) {
            let i;
            const len = data.xf.length;
            for(i = 0; i < len; i += 1) {
              __expression_functions[i] =
                  eval('(function(){ return ' + data.xf[i] + '}())');
            }
          }

          let scoped_bm_rt;
          const expression_function = eval(
              '[function _expression_function(){' + val +
              ';scoped_bm_rt=$bm_rt}' +
              ']')[0];
          const numKeys = property.kf ? data.k.length : 0;

          const active = !this.data || this.data.hd !== true;

          const wiggle = function wiggle(freq, amp) {
            let i;
            let j;
            const len = this.pv.length ? this.pv.length : 1;
            const addedAmps = createTypedArray('float32', len);
            freq = 5;
            const iterations = Math.floor(time * freq);
            i = 0;
            j = 0;
            while(i<iterations){
              // let rnd = BMMath.random();
              for (j = 0; j < len; j += 1) {
                addedAmps[j] += -amp + amp * 2 * BMMath.random();
                // addedAmps[j] += -amp + amp*2*rnd;
              }
              i += 1;
            }
            // let rnd2 = BMMath.random();
            const periods = time * freq;
            const perc = periods - Math.floor(periods);
            const arr = createTypedArray('float32', len);
            if(len>1){
              for (j = 0; j < len; j += 1) {
                arr[j] = this.pv[j] + addedAmps[j] +
                    (-amp + amp * 2 * BMMath.random()) * perc;
                // arr[j] = this.pv[j] + addedAmps[j] + (-amp + amp*2*rnd)*perc;
                // arr[i] = this.pv[i] + addedAmp + amp1*perc + amp2*(1-perc);
              }
              return arr;
            } else {
              return this.pv + addedAmps[0] +
                  (-amp + amp * 2 * BMMath.random()) * perc;
            }
          }.bind(this);

          if (thisProperty.loopIn) {
            loopIn = thisProperty.loopIn.bind(thisProperty);
            loop_in = loopIn;
          }

          if (thisProperty.loopOut) {
            loopOut = thisProperty.loopOut.bind(thisProperty);
            loop_out = loopOut;
          }

          if (thisProperty.smooth) {
            smooth = thisProperty.smooth.bind(thisProperty);
          }

          function loopInDuration(type, duration) {
            return loopIn(type,duration,true);
          }

          function loopOutDuration(type, duration) {
            return loopOut(type,duration,true);
          }

          if (this.getValueAtTime) {
            valueAtTime = this.getValueAtTime.bind(this);
          }

          if (this.getVelocityAtTime) {
            velocityAtTime = this.getVelocityAtTime.bind(this);
          }

          const comp = elem.comp.globalData.projectInterface.bind(
              elem.comp.globalData.projectInterface);

          function lookAt(elem1, elem2) {
            const fVec =
                [elem2[0] - elem1[0], elem2[1] - elem1[1], elem2[2] - elem1[2]];
            const pitch =
                Math.atan2(
                    fVec[0], Math.sqrt(fVec[1] * fVec[1] + fVec[2] * fVec[2])) /
                degToRads;
            const yaw = -Math.atan2(fVec[1], fVec[2]) / degToRads;
            return [yaw,pitch,0];
          }

          function easeOut(t, tMin, tMax, val1, val2) {
            return applyEase(easeOutBez, t, tMin, tMax, val1, val2);
          }

          function easeIn(t, tMin, tMax, val1, val2) {
            return applyEase(easeInBez, t, tMin, tMax, val1, val2);
          }

          function ease(t, tMin, tMax, val1, val2) {
            return applyEase(easeInOutBez, t, tMin, tMax, val1, val2);
          }

          function applyEase(fn, t, tMin, tMax, val1, val2) {
            if(val1 === undefined){
              val1 = tMin;
              val2 = tMax;
            } else {
              t = (t - tMin) / (tMax - tMin);
            }
            t = t > 1 ? 1 : t < 0 ? 0 : t;
            const mult = fn(t);
            if($bm_isInstanceOfArray(val1)) {
              let i;
              const len = val1.length;
              const arr = createTypedArray('float32', len);
              for (i = 0; i < len; i += 1) {
                arr[i] = (val2[i] - val1[i]) * mult + val1[i];
              }
              return arr;
            } else {
              return (val2 - val1) * mult + val1;
            }
          }

          function nearestKey(time) {
            let i;
            const len = data.k.length;
            let index;
            let keyTime;
            if(!data.k.length || typeof(data.k[0]) === 'number'){
              index = 0;
              keyTime = 0;
            } else {
              index = -1;
              time *= elem.comp.globalData.frameRate;
              if (time < data.k[0].t) {
                index = 1;
                keyTime = data.k[0].t;
              } else {
                for (i = 0; i < len - 1; i += 1) {
                  if (time === data.k[i].t) {
                    index = i + 1;
                    keyTime = data.k[i].t;
                    break;
                  } else if (time > data.k[i].t && time < data.k[i + 1].t) {
                    if (time - data.k[i].t > data.k[i + 1].t - time) {
                      index = i + 2;
                      keyTime = data.k[i + 1].t;
                    } else {
                      index = i + 1;
                      keyTime = data.k[i].t;
                    }
                    break;
                  }
                }
                if (index === -1) {
                  index = i + 1;
                  keyTime = data.k[i].t;
                }
              }
            }
            const ob = {};
            ob.index = index;
            ob.time = keyTime/elem.comp.globalData.frameRate;
            return ob;
          }

          function key(ind) {
            let i;
            if(!data.k.length || typeof(data.k[0]) === 'number'){
              throw new Error('The property has no keyframe at index ' + ind);
            }
            ind -= 1;
            const ob = {
              time: data.k[ind].t / elem.comp.globalData.frameRate,
              value: [],
            };
            let arr;
            if(ind === data.k.length - 1 && !data.k[ind].h){
              arr = (data.k[ind].s || data.k[ind].s === 0) ? data.k[ind - 1].s :
                                                             data.k[ind].e;
            }else{
              arr = data.k[ind].s;
            }
            const len = arr.length;
            for(i=0;i<len;i+=1){
              ob[i] = arr[i];
              ob.value[i] = arr[i];
            }
            return ob;
          }

          function framesToTime(frames, fps) {
            if (!fps) {
              fps = elem.comp.globalData.frameRate;
            }
            return frames / fps;
          }

          function timeToFrames(t, fps) {
            if (!t && t !== 0) {
              t = time;
            }
            if (!fps) {
              fps = elem.comp.globalData.frameRate;
            }
            return t * fps;
          }

          function seedRandom(seed) {
            BMMath.seedrandom(randSeed + seed);
          }

          function sourceRectAtTime() {
            return elem.sourceRectAtTime();
          }

          function substring(init, end) {
            if(typeof value === 'string') {
              if (end === undefined) {
                return value.substring(init);
              }
              return value.substring(init, end);
            }
            return '';
          }

          function substr(init, end) {
            if(typeof value === 'string') {
              if (end === undefined) {
                return value.substr(init);
              }
              return value.substr(init, end);
            }
            return '';
          }

          let time;
          let velocity;
          let value;
          let text;
          let textIndex;
          let textTotal;
          let selectorValue;
          const index = elem.data.ind;
          let hasParent = !!(elem.hierarchy && elem.hierarchy.length);
          let parent;
          const randSeed = Math.floor(Math.random() * 1000000);
          const globalData = elem.globalData;
          function executeExpression(_value) {
            // globalData.pushExpression();
            value = _value;
            if (_needsRandom) {
              seedRandom(randSeed);
            }
            if (this.frameExpressionId === elem.globalData.frameId && this.propType !== 'textSelector') {
              return value;
            }
            if(this.propType === 'textSelector'){
              textIndex = this.textIndex;
              textTotal = this.textTotal;
              selectorValue = this.selectorValue;
            }
            if (!thisLayer) {
              text = elem.layerInterface.text;
              thisLayer = elem.layerInterface;
              thisComp = elem.comp.compInterface;
              toWorld = thisLayer.toWorld.bind(thisLayer);
              fromWorld = thisLayer.fromWorld.bind(thisLayer);
              fromComp = thisLayer.fromComp.bind(thisLayer);
              toComp = thisLayer.toComp.bind(thisLayer);
              mask = thisLayer.mask ? thisLayer.mask.bind(thisLayer) : null;
              fromCompToSurface = fromComp;
            }
            if (!transform) {
              transform = elem.layerInterface('ADBE Transform Group');
              $bm_transform = transform;
              if (transform) {
                anchorPoint = transform.anchorPoint;
                /*position = transform.position;
                rotation = transform.rotation;
                scale = transform.scale;*/
              }
            }

            if (elemType === 4 && !content) {
              content = thisLayer('ADBE Root Vectors Group');
            }
            if (!effect) {
              effect = thisLayer(4);
            }
            hasParent = !!(elem.hierarchy && elem.hierarchy.length);
            if (hasParent && !parent) {
              parent = elem.hierarchy[0].layerInterface;
            }
            time = this.comp.renderedFrame/this.comp.globalData.frameRate;
            if (needsVelocity) {
              velocity = velocityAtTime(time);
            }
            expression_function();
            this.frameExpressionId = elem.globalData.frameId;


            //TODO: Check if it's possible to return on ShapeInterface the .v value
            if (scoped_bm_rt.propType === 'shape') {
              scoped_bm_rt = scoped_bm_rt.v;
            }
            // globalData.popExpression();
            return scoped_bm_rt;
          }
          return executeExpression;
        }

        ob.initiateExpression = initiateExpression;
        return ob;
      }());
  const expressionHelpers =
      (function() {
        function searchExpressions(elem, data, prop) {
          if (data.x) {
            prop.k = true;
            prop.x = true;
            prop.initiateExpression = ExpressionManager.initiateExpression;
            prop.effectsSequence.push(prop.initiateExpression(elem,data,prop).bind(prop));
          }
        }

        function getValueAtTime(frameNum) {
          frameNum *= this.elem.globalData.frameRate;
          frameNum -= this.offsetTime;
          if (frameNum !== this._cachingAtTime.lastFrame) {
            this._cachingAtTime.lastIndex = this._cachingAtTime.lastFrame < frameNum ? this._cachingAtTime.lastIndex : 0;
            this._cachingAtTime.value = this.interpolateValue(frameNum, this._cachingAtTime);
            this._cachingAtTime.lastFrame = frameNum;
          }
          return this._cachingAtTime.value;
        }

        function getSpeedAtTime(frameNum) {
          const delta = -0.01;
          const v1 = this.getValueAtTime(frameNum);
          const v2 = this.getValueAtTime(frameNum + delta);
          let speed = 0;
          if (v1.length) {
            let i;
            for(i=0;i<v1.length;i+=1){
              speed += Math.pow(v2[i] - v1[i], 2);
            }
            speed = Math.sqrt(speed) * 100;
          } else {
            speed = 0;
          }
          return speed;
        }

        function getVelocityAtTime(frameNum) {
          if (this.vel !== undefined) {
            return this.vel;
          }
          const delta = -0.001;
          // frameNum += this.elem.data.st;
          const v1 = this.getValueAtTime(frameNum);
          const v2 = this.getValueAtTime(frameNum + delta);
          let velocity;
          if (v1.length) {
            velocity = createTypedArray('float32', v1.length);
            let i;
            for(i=0;i<v1.length;i+=1){
              // removing frameRate
              // if needed, don't add it here
              // velocity[i] = this.elem.globalData.frameRate*((v2[i] -
              // v1[i])/delta);
              velocity[i] = (v2[i] - v1[i]) / delta;
            }
          } else {
            velocity = (v2 - v1)/delta;
          }
          return velocity;
        }

        function getStaticValueAtTime() {
          return this.pv;
        }

        function setGroupProperty(propertyGroup) {
          this.propertyGroup = propertyGroup;
        }

        return {
          searchExpressions: searchExpressions,
          getSpeedAtTime: getSpeedAtTime,
          getVelocityAtTime: getVelocityAtTime,
          getValueAtTime: getValueAtTime,
          getStaticValueAtTime: getStaticValueAtTime,
          setGroupProperty: setGroupProperty,
        };
      }());
  (
      function addPropertyDecorator() {
        function loopOut(type, duration, durationFlag) {
          if (!this.k || !this.keyframes) {
            return this.pv;
          }
          type = type ? type.toLowerCase() : '';
          const currentFrame = this.comp.renderedFrame;
          const keyframes = this.keyframes;
          const lastKeyFrame = keyframes[keyframes.length - 1].t;
          if (currentFrame <= lastKeyFrame) {
            return this.pv;
          } else {
            let cycleDuration;
            let firstKeyFrame;
            if(!durationFlag){
              if (!duration || duration > keyframes.length - 1) {
                duration = keyframes.length - 1;
              }
              firstKeyFrame = keyframes[keyframes.length - 1 - duration].t;
              cycleDuration = lastKeyFrame - firstKeyFrame;
            } else {
              if (!duration) {
                cycleDuration = Math.max(0, lastKeyFrame - this.elem.data.ip);
              } else {
                cycleDuration = Math.abs(
                    lastKeyFrame - elem.comp.globalData.frameRate * duration);
              }
              firstKeyFrame = lastKeyFrame - cycleDuration;
            }
            let i;
            let len;
            let ret;
            if(type === 'pingpong') {
              const iterations =
                  Math.floor((currentFrame - firstKeyFrame) / cycleDuration);
              if (iterations % 2 !== 0) {
                return this.getValueAtTime(
                    ((cycleDuration -
                      (currentFrame - firstKeyFrame) % cycleDuration +
                      firstKeyFrame)) /
                        this.comp.globalData.frameRate,
                    0);
              }
            } else if(type === 'offset'){
              const initV = this.getValueAtTime(
                  firstKeyFrame / this.comp.globalData.frameRate, 0);
              const endV = this.getValueAtTime(
                  lastKeyFrame / this.comp.globalData.frameRate, 0);
              const current = this.getValueAtTime(
                  ((currentFrame - firstKeyFrame) % cycleDuration +
                   firstKeyFrame) /
                      this.comp.globalData.frameRate,
                  0);
              const repeats =
                  Math.floor((currentFrame - firstKeyFrame) / cycleDuration);
              if (this.pv.length) {
                ret = new Array(initV.length);
                len = ret.length;
                for (i = 0; i < len; i += 1) {
                  ret[i] = (endV[i] - initV[i]) * repeats + current[i];
                }
                return ret;
              }
              return (endV - initV) * repeats + current;
            } else if(type === 'continue'){
              const lastValue = this.getValueAtTime(
                  lastKeyFrame / this.comp.globalData.frameRate, 0);
              const nextLastValue = this.getValueAtTime(
                  (lastKeyFrame - 0.001) / this.comp.globalData.frameRate, 0);
              if (this.pv.length) {
                ret = new Array(lastValue.length);
                len = ret.length;
                for (i = 0; i < len; i += 1) {
                  ret[i] = lastValue[i] +
                      (lastValue[i] - nextLastValue[i]) *
                          ((currentFrame - lastKeyFrame) /
                           this.comp.globalData.frameRate) /
                          0.0005;
                }
                return ret;
              }
              return lastValue +
                  (lastValue - nextLastValue) *
                  (((currentFrame - lastKeyFrame)) / 0.001);
            }
            return this.getValueAtTime((((currentFrame - firstKeyFrame) % cycleDuration +  firstKeyFrame)) / this.comp.globalData.frameRate, 0);
          }
        }

        function loopIn(type, duration, durationFlag) {
          if (!this.k) {
            return this.pv;
          }
          type = type ? type.toLowerCase() : '';
          const currentFrame = this.comp.renderedFrame;
          const keyframes = this.keyframes;
          const firstKeyFrame = keyframes[0].t;
          if (currentFrame >= firstKeyFrame) {
            return this.pv;
          } else {
            let cycleDuration;
            let lastKeyFrame;
            if(!durationFlag){
              if (!duration || duration > keyframes.length - 1) {
                duration = keyframes.length - 1;
              }
              lastKeyFrame = keyframes[duration].t;
              cycleDuration = lastKeyFrame - firstKeyFrame;
            } else {
              if (!duration) {
                cycleDuration = Math.max(0, this.elem.data.op - firstKeyFrame);
              } else {
                cycleDuration =
                    Math.abs(elem.comp.globalData.frameRate * duration);
              }
              lastKeyFrame = firstKeyFrame + cycleDuration;
            }
            let i;
            let len;
            let ret;
            if(type === 'pingpong') {
              const iterations =
                  Math.floor((firstKeyFrame - currentFrame) / cycleDuration);
              if (iterations % 2 === 0) {
                return this.getValueAtTime(
                    (((firstKeyFrame - currentFrame) % cycleDuration +
                      firstKeyFrame)) /
                        this.comp.globalData.frameRate,
                    0);
              }
            } else if(type === 'offset'){
              const initV = this.getValueAtTime(
                  firstKeyFrame / this.comp.globalData.frameRate, 0);
              const endV = this.getValueAtTime(
                  lastKeyFrame / this.comp.globalData.frameRate, 0);
              const current = this.getValueAtTime(
                  (cycleDuration -
                   (firstKeyFrame - currentFrame) % cycleDuration +
                   firstKeyFrame) /
                      this.comp.globalData.frameRate,
                  0);
              const repeats =
                  Math.floor((firstKeyFrame - currentFrame) / cycleDuration) +
                  1;
              if (this.pv.length) {
                ret = new Array(initV.length);
                len = ret.length;
                for (i = 0; i < len; i += 1) {
                  ret[i] = current[i] - (endV[i] - initV[i]) * repeats;
                }
                return ret;
              }
              return current - (endV - initV) * repeats;
            } else if(type === 'continue'){
              const firstValue = this.getValueAtTime(
                  firstKeyFrame / this.comp.globalData.frameRate, 0);
              const nextFirstValue = this.getValueAtTime(
                  (firstKeyFrame + 0.001) / this.comp.globalData.frameRate, 0);
              if (this.pv.length) {
                ret = new Array(firstValue.length);
                len = ret.length;
                for (i = 0; i < len; i += 1) {
                  ret[i] = firstValue[i] +
                      (firstValue[i] - nextFirstValue[i]) *
                          (firstKeyFrame - currentFrame) / 0.001;
                }
                return ret;
              }
              return firstValue +
                  (firstValue - nextFirstValue) *
                  (firstKeyFrame - currentFrame) / 0.001;
            }
            return this.getValueAtTime(((cycleDuration - (firstKeyFrame - currentFrame) % cycleDuration +  firstKeyFrame)) / this.comp.globalData.frameRate, 0);
          }
        }

        function smooth(width, samples) {
          if (!this.k) {
            return this.pv;
          }
          width = (width || 0.4) * 0.5;
          samples = Math.floor(samples || 5);
          if (samples <= 1) {
            return this.pv;
          }
          const currentTime =
              this.comp.renderedFrame / this.comp.globalData.frameRate;
          const initFrame = currentTime - width;
          const endFrame = currentTime + width;
          const sampleFrequency =
              samples > 1 ? (endFrame - initFrame) / (samples - 1) : 1;
          let i = 0;
          let j = 0;
          let value;
          if (this.pv.length) {
            value = createTypedArray('float32', this.pv.length);
          } else {
            value = 0;
          }
          let sampleValue;
          while (i < samples) {
            sampleValue = this.getValueAtTime(initFrame + i * sampleFrequency);
            if(this.pv.length) {
              for (j = 0; j < this.pv.length; j += 1) {
                value[j] += sampleValue[j];
              }
            } else {
              value += sampleValue;
            }
            i += 1;
          }
          if (this.pv.length) {
            for (j = 0; j < this.pv.length; j += 1) {
              value[j] /= samples;
            }
          } else {
            value /= samples;
          }
          return value;
        }

        function getValueAtTime(frameNum) {
          frameNum *= this.elem.globalData.frameRate;
          frameNum -= this.offsetTime;
          if (frameNum !== this._cachingAtTime.lastFrame) {
            this._cachingAtTime.lastIndex = this._cachingAtTime.lastFrame < frameNum ? this._cachingAtTime.lastIndex : 0;
            this._cachingAtTime.value = this.interpolateValue(frameNum, this._cachingAtTime);
            this._cachingAtTime.lastFrame = frameNum;
          }
          return this._cachingAtTime.value;
        }

        function getTransformValueAtTime(time) {
          console.warn('Transform at time not supported');
        }

        function getTransformStaticValueAtTime(time) {}

        const getTransformProperty =
            TransformPropertyFactory.getTransformProperty;
        TransformPropertyFactory.getTransformProperty = function(
            elem, data, container) {
          const prop = getTransformProperty(elem, data, container);
          if (prop.dynamicProperties.length) {
            prop.getValueAtTime = getTransformValueAtTime.bind(prop);
          } else {
            prop.getValueAtTime = getTransformStaticValueAtTime.bind(prop);
          }
          prop.setGroupProperty = expressionHelpers.setGroupProperty;
          return prop;
        };

        const propertyGetProp = PropertyFactory.getProp;
        PropertyFactory.getProp = function(elem, data, type, mult, container) {
          const prop = propertyGetProp(elem, data, type, mult, container);
          // prop.getVelocityAtTime = getVelocityAtTime;
          // prop.loopOut = loopOut;
          // prop.loopIn = loopIn;
          if (prop.kf) {
            prop.getValueAtTime = expressionHelpers.getValueAtTime.bind(prop);
          } else {
            prop.getValueAtTime = expressionHelpers.getStaticValueAtTime.bind(prop);
          }
          prop.setGroupProperty = expressionHelpers.setGroupProperty;
          prop.loopOut = loopOut;
          prop.loopIn = loopIn;
          prop.smooth = smooth;
          prop.getVelocityAtTime =
              expressionHelpers.getVelocityAtTime.bind(prop);
          prop.getSpeedAtTime = expressionHelpers.getSpeedAtTime.bind(prop);
          prop.numKeys = data.a === 1 ? data.k.length : 0;
          prop.propertyIndex = data.ix;
          let value = 0;
          if (type !== 0) {
            value = createTypedArray('float32', data.a === 1 ?  data.k[0].s.length : data.k.length);
          }
          prop._cachingAtTime = {
            lastFrame: initialDefaultFrame,
            lastIndex: 0,
            value: value,
          };
          expressionHelpers.searchExpressions(elem, data, prop);
          if (prop.k) {
            container.addDynamicProperty(prop);
          }

          return prop;
        };

        function getShapeValueAtTime(frameNum) {
          // For now this caching object is created only when needed instead of
          // creating it when the shape is initialized.
          if (!this._cachingAtTime) {
            this._cachingAtTime = {
              shapeValue: shape_pool.clone(this.pv),
              lastIndex: 0,
              lastTime: initialDefaultFrame,
            };
          }

          frameNum *= this.elem.globalData.frameRate;
          frameNum -= this.offsetTime;
          if (frameNum !== this._cachingAtTime.lastTime) {
            this._cachingAtTime.lastIndex = this._cachingAtTime.lastTime < frameNum ? this._caching.lastIndex : 0;
            this._cachingAtTime.lastTime = frameNum;
            this.interpolateShape(frameNum, this._cachingAtTime.shapeValue, this._cachingAtTime);
          }
          return this._cachingAtTime.shapeValue;
        }

        const ShapePropertyConstructorFunction =
            ShapePropertyFactory.getConstructorFunction();
        const KeyframedShapePropertyConstructorFunction =
            ShapePropertyFactory.getKeyframedConstructorFunction();

        function ShapeExpressions() {}
        ShapeExpressions.prototype =
            {
              vertices: function(prop, time) {
                if (this.k) {
                  this.getValue();
                }
                let shapePath = this.v;
                if (time !== undefined) {
                  shapePath = this.getValueAtTime(time, 0);
                }
                let i;
                const len = shapePath._length;
                const vertices = shapePath[prop];
                const points = shapePath.v;
                const arr = createSizedArray(len);
                for (i = 0; i < len; i += 1) {
                  if (prop === 'i' || prop === 'o') {
                    arr[i] = [vertices[i][0] - points[i][0], vertices[i][1] - points[i][1]];
                  } else {
                    arr[i] = [vertices[i][0], vertices[i][1]];
                  }
                }
                return arr;
              },
              points: function(time) {
                return this.vertices('v', time);
              },
              inTangents: function(time) {
                return this.vertices('i', time);
              },
              outTangents: function(time) {
                return this.vertices('o', time);
              },
              isClosed: function() {
                return this.v.c;
              },
              pointOnPath: function(perc, time) {
                let shapePath = this.v;
                if (time !== undefined) {
                  shapePath = this.getValueAtTime(time, 0);
                }
                if (!this._segmentsLength) {
                  this._segmentsLength = bez.getSegmentsLength(shapePath);
                }

                const segmentsLength = this._segmentsLength;
                const lengths = segmentsLength.lengths;
                const lengthPos = segmentsLength.totalLength * perc;
                let i = 0;
                const len = lengths.length;
                const j = 0;
                let jLen;
                let accumulatedLength = 0;
                let pt;
                while (i < len) {
                  if (accumulatedLength + lengths[i].addedLength > lengthPos) {
                    const initIndex = i;
                    const endIndex = (shapePath.c && i === len - 1) ? 0 : i + 1;
                    const segmentPerc = (lengthPos - accumulatedLength) /
                        lengths[i].addedLength;
                    pt = bez.getPointInSegment(shapePath.v[initIndex], shapePath.v[endIndex], shapePath.o[initIndex], shapePath.i[endIndex], segmentPerc, lengths[i]);
                    break;
                  } else {
                    accumulatedLength += lengths[i].addedLength;
                  }
                  i += 1;
                }
                if (!pt) {
                  pt = shapePath.c ? [shapePath.v[0][0], shapePath.v[0][1]] : [
                    shapePath.v[shapePath._length - 1][0],
                    shapePath.v[shapePath._length - 1][1],
                  ];
                }
                return pt;
              },
              vectorOnPath: function(perc, time, vectorType) {
                // perc doesn't use triple equality because it can be a Number
                // object as well as a primitive.
                perc = perc == 1 ? this.v.c ? 0 : 0.999 : perc;
                const pt1 = this.pointOnPath(perc, time);
                const pt2 = this.pointOnPath(perc + 0.001, time);
                const xLength = pt2[0] - pt1[0];
                const yLength = pt2[1] - pt1[1];
                const magnitude =
                    Math.sqrt(Math.pow(xLength, 2) + Math.pow(yLength, 2));
                const unitVector = vectorType === 'tangent' ?
                    [xLength / magnitude, yLength / magnitude] :
                    [-yLength / magnitude, xLength / magnitude];
                return unitVector;
              },
              tangentOnPath: function(perc, time) {
                return this.vectorOnPath(perc, time, 'tangent');
              },
              normalOnPath: function(perc, time) {
                return this.vectorOnPath(perc, time, 'normal');
              },
              setGroupProperty: expressionHelpers.setGroupProperty,
              getValueAtTime: expressionHelpers.getStaticValueAtTime,
            };
        extendPrototype([ShapeExpressions], ShapePropertyConstructorFunction);
        extendPrototype(
            [ShapeExpressions], KeyframedShapePropertyConstructorFunction);
        KeyframedShapePropertyConstructorFunction.prototype.getValueAtTime =
            getShapeValueAtTime;
        KeyframedShapePropertyConstructorFunction.prototype.initiateExpression =
            ExpressionManager.initiateExpression;

        const propertyGetShapeProp = ShapePropertyFactory.getShapeProp;
        ShapePropertyFactory.getShapeProp = function(
            elem, data, type, arr, trims) {
          const prop = propertyGetShapeProp(elem, data, type, arr, trims);
          prop.propertyIndex = data.ix;
          prop.lock = false;
          if (type === 3) {
            expressionHelpers.searchExpressions(elem,data.pt,prop);
          } else if (type === 4) {
            expressionHelpers.searchExpressions(elem,data.ks,prop);
          }
          if (prop.k) {
            elem.addDynamicProperty(prop);
          }
          return prop;
        };
      }());
  (function addDecorator() {
    function searchExpressions(){
      if (this.data.d.x) {
        this.calculateExpression = ExpressionManager.initiateExpression.bind(
            this)(this.elem, this.data.d, this);
        this.addEffect(this.getExpressionValue.bind(this));
        return true;
      }
    }

    TextProperty.prototype.getExpressionValue = function(currentValue, text) {
      const newValue = this.calculateExpression(text);
      if (currentValue.t !== newValue) {
        const newData = {};
        this.copyData(newData, currentValue);
        newData.t = newValue.toString();
        newData.__complete = false;
        return newData;
      }
      return currentValue;
    };

    TextProperty.prototype.searchProperty = function() {
      const isKeyframed = this.searchKeyframes();
      const hasExpressions = this.searchExpressions();
      this.kf = isKeyframed || hasExpressions;
      return this.kf;
    };

    TextProperty.prototype.searchExpressions = searchExpressions;
  }());
  const ShapeExpressionInterface = (function() {
    function iterateElements(shapes,view, propertyGroup){
      const arr = [];
      let i;
      const len = shapes ? shapes.length : 0;
      for (i = 0; i < len; i += 1) {
        if (shapes[i].ty == 'gr') {
          arr.push(groupInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'fl') {
          arr.push(fillInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'st') {
          arr.push(strokeInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'tm') {
          arr.push(trimInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'tr') {
          // arr.push(transformInterfaceFactory(shapes[i],view[i],propertyGroup));
        } else if (shapes[i].ty == 'el') {
          arr.push(ellipseInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'sr') {
          arr.push(starInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'sh') {
          arr.push(pathInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'rc') {
          arr.push(rectInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'rd') {
          arr.push(roundedInterfaceFactory(shapes[i], view[i], propertyGroup));
        } else if (shapes[i].ty == 'rp') {
          arr.push(repeaterInterfaceFactory(shapes[i], view[i], propertyGroup));
        }
      }
      return arr;
    }

    function contentsInterfaceFactory(shape,view, propertyGroup){
      let interfaces = undefined;
      const interfaceFunction = function _interfaceFunction(value) {
        let i = 0;
        const len = interfaces.length;
        while (i < len) {
          if (interfaces[i]._name === value || interfaces[i].mn === value ||
              interfaces[i].propertyIndex === value ||
              interfaces[i].ix === value || interfaces[i].ind === value) {
            return interfaces[i];
          }
          i += 1;
        }
        if (typeof value === 'number') {
          return interfaces[value - 1];
        }
      };
      interfaceFunction.propertyGroup = function(val) {
        if (val === 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(val - 1);
        }
      };
      interfaces =
          iterateElements(shape.it, view.it, interfaceFunction.propertyGroup);
      interfaceFunction.numProperties = interfaces.length;
      interfaceFunction.propertyIndex = shape.cix;
      interfaceFunction._name = shape.nm;

      return interfaceFunction;
    }

    function groupInterfaceFactory(shape,view, propertyGroup){
      const interfaceFunction = function _interfaceFunction(value) {
        switch (value) {
          case 'ADBE Vectors Group':
          case 'Contents':
          case 2:
            return interfaceFunction.content;
          // Not necessary for now. Keeping them here in case a new case appears
          // case 'ADBE Vector Transform Group':
          // case 3:
          default:
            return interfaceFunction.transform;
        }
      };
      interfaceFunction.propertyGroup = function(val) {
        if (val === 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(val - 1);
        }
      };
      const content = contentsInterfaceFactory(
          shape, view, interfaceFunction.propertyGroup);
      const transformInterface = transformInterfaceFactory(
          shape.it[shape.it.length - 1], view.it[view.it.length - 1],
          interfaceFunction.propertyGroup);
      interfaceFunction.content = content;
      interfaceFunction.transform = transformInterface;
      Object.defineProperty(interfaceFunction, '_name', {
        get: function() {
          return shape.nm;
        },
      });
      // interfaceFunction.content = interfaceFunction;
      interfaceFunction.numProperties = shape.np;
      interfaceFunction.propertyIndex = shape.ix;
      interfaceFunction.nm = shape.nm;
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function fillInterfaceFactory(shape,view,propertyGroup){
      function interfaceFunction(val) {
        if (val === 'Color' || val === 'color') {
          return interfaceFunction.color;
        } else if (val === 'Opacity' || val === 'opacity') {
          return interfaceFunction.opacity;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'color': {
          get: ExpressionPropertyInterface(view.c),
        },
        'opacity': {
          get: ExpressionPropertyInterface(view.o),
        },
        '_name': {value: shape.nm},
        'mn': {value: shape.mn},
      });

      view.c.setGroupProperty(propertyGroup);
      view.o.setGroupProperty(propertyGroup);
      return interfaceFunction;
    }

    function strokeInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val === 1) {
          return ob;
        } else {
          return propertyGroup(val - 1);
        }
      }
      function _dashPropertyGroup(val) {
        if (val === 1) {
          return dashOb;
        } else {
          return _propertyGroup(val - 1);
        }
      }
      function addPropertyToDashOb(i) {
        Object.defineProperty(dashOb, shape.d[i].nm, {
          get: ExpressionPropertyInterface(view.d.dataProps[i].p),
        });
      }
      let i;
      const len = shape.d ? shape.d.length : 0;
      const dashOb = {};
      for (i = 0; i < len; i += 1) {
        addPropertyToDashOb(i);
        view.d.dataProps[i].p.setGroupProperty(_dashPropertyGroup);
      }

      function interfaceFunction(val) {
        if (val === 'Color' || val === 'color') {
          return interfaceFunction.color;
        } else if (val === 'Opacity' || val === 'opacity') {
          return interfaceFunction.opacity;
        } else if (val === 'Stroke Width' || val === 'stroke width') {
          return interfaceFunction.strokeWidth;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'color': {
          get: ExpressionPropertyInterface(view.c),
        },
        'opacity': {
          get: ExpressionPropertyInterface(view.o),
        },
        'strokeWidth': {
          get: ExpressionPropertyInterface(view.w),
        },
        'dash': {
          get: function() {
            return dashOb;
          },
        },
        '_name': {value: shape.nm},
        'mn': {value: shape.mn},
      });

      view.c.setGroupProperty(_propertyGroup);
      view.o.setGroupProperty(_propertyGroup);
      view.w.setGroupProperty(_propertyGroup);
      return interfaceFunction;
    }

    function trimInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      interfaceFunction.propertyIndex = shape.ix;

      view.s.setGroupProperty(_propertyGroup);
      view.e.setGroupProperty(_propertyGroup);
      view.o.setGroupProperty(_propertyGroup);

      function interfaceFunction(val) {
        if (val === shape.e.ix || val === 'End' || val === 'end') {
          return interfaceFunction.end;
        }
        if (val === shape.s.ix) {
          return interfaceFunction.start;
        }
        if (val === shape.o.ix) {
          return interfaceFunction.offset;
        }
      }
      interfaceFunction.propertyIndex = shape.ix;
      interfaceFunction.propertyGroup = propertyGroup;

      Object.defineProperties(interfaceFunction, {
        'start': {
          get: ExpressionPropertyInterface(view.s),
        },
        'end': {
          get: ExpressionPropertyInterface(view.e),
        },
        'offset': {
          get: ExpressionPropertyInterface(view.o),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function transformInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      view.transform.mProps.o.setGroupProperty(_propertyGroup);
      view.transform.mProps.p.setGroupProperty(_propertyGroup);
      view.transform.mProps.a.setGroupProperty(_propertyGroup);
      view.transform.mProps.s.setGroupProperty(_propertyGroup);
      view.transform.mProps.r.setGroupProperty(_propertyGroup);
      if (view.transform.mProps.sk) {
        view.transform.mProps.sk.setGroupProperty(_propertyGroup);
        view.transform.mProps.sa.setGroupProperty(_propertyGroup);
      }
      view.transform.op.setGroupProperty(_propertyGroup);

      function interfaceFunction(value) {
        if (shape.a.ix === value || value === 'Anchor Point') {
          return interfaceFunction.anchorPoint;
        }
        if (shape.o.ix === value || value === 'Opacity') {
          return interfaceFunction.opacity;
        }
        if (shape.p.ix === value || value === 'Position') {
          return interfaceFunction.position;
        }
        if (shape.r.ix === value || value === 'Rotation' ||
            value === 'ADBE Vector Rotation') {
          return interfaceFunction.rotation;
        }
        if (shape.s.ix === value || value === 'Scale') {
          return interfaceFunction.scale;
        }
        if (shape.sk && shape.sk.ix === value || value === 'Skew') {
          return interfaceFunction.skew;
        }
        if (shape.sa && shape.sa.ix === value || value === 'Skew Axis') {
          return interfaceFunction.skewAxis;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'opacity': {
          get: ExpressionPropertyInterface(view.transform.mProps.o),
        },
        'position': {
          get: ExpressionPropertyInterface(view.transform.mProps.p),
        },
        'anchorPoint': {
          get: ExpressionPropertyInterface(view.transform.mProps.a),
        },
        'scale': {
          get: ExpressionPropertyInterface(view.transform.mProps.s),
        },
        'rotation': {
          get: ExpressionPropertyInterface(view.transform.mProps.r),
        },
        'skew': {
          get: ExpressionPropertyInterface(view.transform.mProps.sk),
        },
        'skewAxis': {
          get: ExpressionPropertyInterface(view.transform.mProps.sa),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.ty = 'tr';
      interfaceFunction.mn = shape.mn;
      interfaceFunction.propertyGroup = propertyGroup;
      return interfaceFunction;
    }

    function ellipseInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      interfaceFunction.propertyIndex = shape.ix;
      const prop = view.sh.ty === 'tm' ? view.sh.prop : view.sh;
      prop.s.setGroupProperty(_propertyGroup);
      prop.p.setGroupProperty(_propertyGroup);
      function interfaceFunction(value) {
        if (shape.p.ix === value) {
          return interfaceFunction.position;
        }
        if (shape.s.ix === value) {
          return interfaceFunction.size;
        }
      }

      Object.defineProperties(interfaceFunction, {
        'size': {
          get: ExpressionPropertyInterface(prop.s),
        },
        'position': {
          get: ExpressionPropertyInterface(prop.p),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function starInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      const prop = view.sh.ty === 'tm' ? view.sh.prop : view.sh;
      interfaceFunction.propertyIndex = shape.ix;
      prop.or.setGroupProperty(_propertyGroup);
      prop.os.setGroupProperty(_propertyGroup);
      prop.pt.setGroupProperty(_propertyGroup);
      prop.p.setGroupProperty(_propertyGroup);
      prop.r.setGroupProperty(_propertyGroup);
      if (shape.ir) {
        prop.ir.setGroupProperty(_propertyGroup);
        prop.is.setGroupProperty(_propertyGroup);
      }

      function interfaceFunction(value) {
        if (shape.p.ix === value) {
          return interfaceFunction.position;
        }
        if (shape.r.ix === value) {
          return interfaceFunction.rotation;
        }
        if (shape.pt.ix === value) {
          return interfaceFunction.points;
        }
        if (shape.or.ix === value ||
            'ADBE Vector Star Outer Radius' === value) {
          return interfaceFunction.outerRadius;
        }
        if (shape.os.ix === value) {
          return interfaceFunction.outerRoundness;
        }
        if (shape.ir &&
            (shape.ir.ix === value ||
             'ADBE Vector Star Inner Radius' === value)) {
          return interfaceFunction.innerRadius;
        }
        if (shape.is && shape.is.ix === value) {
          return interfaceFunction.innerRoundness;
        }
      }

      Object.defineProperties(interfaceFunction, {
        'position': {
          get: ExpressionPropertyInterface(prop.p),
        },
        'rotation': {
          get: ExpressionPropertyInterface(prop.r),
        },
        'points': {
          get: ExpressionPropertyInterface(prop.pt),
        },
        'outerRadius': {
          get: ExpressionPropertyInterface(prop.or),
        },
        'outerRoundness': {
          get: ExpressionPropertyInterface(prop.os),
        },
        'innerRadius': {
          get: ExpressionPropertyInterface(prop.ir),
        },
        'innerRoundness': {
          get: ExpressionPropertyInterface(prop.is),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function rectInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      const prop = view.sh.ty === 'tm' ? view.sh.prop : view.sh;
      interfaceFunction.propertyIndex = shape.ix;
      prop.p.setGroupProperty(_propertyGroup);
      prop.s.setGroupProperty(_propertyGroup);
      prop.r.setGroupProperty(_propertyGroup);

      function interfaceFunction(value) {
        if (shape.p.ix === value) {
          return interfaceFunction.position;
        }
        if (shape.r.ix === value) {
          return interfaceFunction.roundness;
        }
        if (shape.s.ix === value || value === 'Size' ||
            value === 'ADBE Vector Rect Size') {
          return interfaceFunction.size;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'position': {
          get: ExpressionPropertyInterface(prop.p),
        },
        'roundness': {
          get: ExpressionPropertyInterface(prop.r),
        },
        'size': {
          get: ExpressionPropertyInterface(prop.s),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function roundedInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      const prop = view;
      interfaceFunction.propertyIndex = shape.ix;
      prop.rd.setGroupProperty(_propertyGroup);

      function interfaceFunction(value) {
        if (shape.r.ix === value || 'Round Corners 1' === value) {
          return interfaceFunction.radius;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'radius': {
          get: ExpressionPropertyInterface(prop.rd),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function repeaterInterfaceFactory(shape,view,propertyGroup){
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      const prop = view;
      interfaceFunction.propertyIndex = shape.ix;
      prop.c.setGroupProperty(_propertyGroup);
      prop.o.setGroupProperty(_propertyGroup);

      function interfaceFunction(value) {
        if (shape.c.ix === value || 'Copies' === value) {
          return interfaceFunction.copies;
        } else if (shape.o.ix === value || 'Offset' === value) {
          return interfaceFunction.offset;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'copies': {
          get: ExpressionPropertyInterface(prop.c),
        },
        'offset': {
          get: ExpressionPropertyInterface(prop.o),
        },
        '_name': {value: shape.nm},
      });
      interfaceFunction.mn = shape.mn;
      return interfaceFunction;
    }

    function pathInterfaceFactory(shape,view,propertyGroup){
      const prop = view.sh;
      function _propertyGroup(val) {
        if (val == 1) {
          return interfaceFunction;
        } else {
          return propertyGroup(--val);
        }
      }
      prop.setGroupProperty(_propertyGroup);

      function interfaceFunction(val) {
        if (val === 'Shape' || val === 'shape' || val === 'Path' ||
            val === 'path' || val === 'ADBE Vector Shape' || val === 2) {
          return interfaceFunction.path;
        }
      }
      Object.defineProperties(interfaceFunction, {
        'path': {
          get: function() {
            if (prop.k) {
              prop.getValue();
            }
            return prop;
          },
        },
        'shape': {
          get: function() {
            if (prop.k) {
              prop.getValue();
            }
            return prop;
          },
        },
        '_name': {value: shape.nm},
        'ix': {value: shape.ix},
        'mn': {value: shape.mn},
      });
      return interfaceFunction;
    }

    return function(shapes, view, propertyGroup) {
      let interfaces = undefined;
      function _interfaceFunction(value) {
        if (typeof value === 'number') {
          return interfaces[value - 1];
        } else {
          let i = 0;
          const len = interfaces.length;
          while (i < len) {
            if (interfaces[i]._name === value) {
              return interfaces[i];
            }
            i += 1;
          }
        }
      }
      _interfaceFunction.propertyGroup = propertyGroup;
      interfaces = iterateElements(shapes, view, _interfaceFunction);
      _interfaceFunction.numProperties = interfaces.length;
      return _interfaceFunction;
    };
  }());

  const TextExpressionInterface = (function() {
    return function(elem) {
      let _prevValue;
      let _sourceText;
      function _thisLayerFunction() {}
      Object.defineProperty(_thisLayerFunction, 'sourceText', {
        get: function() {
          elem.textProperty.getValue();
          const stringValue = elem.textProperty.currentData.t;
          if (stringValue !== _prevValue) {
            elem.textProperty.currentData.t = _prevValue;
            // eslint-disable-next-line no-new-wrappers
            _sourceText = new String(stringValue);
            // If stringValue is an empty string, eval returns undefined, so it
            // has to be returned as a String primitive
            _sourceText.value =
                // eslint-disable-next-line no-new-wrappers
                stringValue ? stringValue : new String(stringValue);
          }
          return _sourceText;
        },
      });
      return _thisLayerFunction;
    };
  }());
  const LayerExpressionInterface = (function() {
    function toWorld(arr, time){
      const toWorldMat = new Matrix();
      toWorldMat.reset();
      let transformMat;
      if (time) {
        // Todo implement value at time on transform properties
        // transformMat = this._elem.finalTransform.mProp.getValueAtTime(time);
        transformMat = this._elem.finalTransform.mProp;
      } else {
        transformMat = this._elem.finalTransform.mProp;
      }
      transformMat.applyToMatrix(toWorldMat);
      if (this._elem.hierarchy && this._elem.hierarchy.length) {
        let i;
        const len = this._elem.hierarchy.length;
        for (i = 0; i < len; i += 1) {
          this._elem.hierarchy[i].finalTransform.mProp.applyToMatrix(
              toWorldMat);
        }
        return toWorldMat.applyToPointArray(arr[0], arr[1], arr[2] || 0);
      }
      return toWorldMat.applyToPointArray(arr[0], arr[1], arr[2] || 0);
    }
    function fromWorld(arr, time){
      const toWorldMat = new Matrix();
      toWorldMat.reset();
      let transformMat;
      if (time) {
        // Todo implement value at time on transform properties
        // transformMat = this._elem.finalTransform.mProp.getValueAtTime(time);
        transformMat = this._elem.finalTransform.mProp;
      } else {
        transformMat = this._elem.finalTransform.mProp;
      }
      transformMat.applyToMatrix(toWorldMat);
      if (this._elem.hierarchy && this._elem.hierarchy.length) {
        let i;
        const len = this._elem.hierarchy.length;
        for (i = 0; i < len; i += 1) {
          this._elem.hierarchy[i].finalTransform.mProp.applyToMatrix(
              toWorldMat);
        }
        return toWorldMat.inversePoint(arr);
      }
      return toWorldMat.inversePoint(arr);
    }
    function fromComp(arr){
      const toWorldMat = new Matrix();
      toWorldMat.reset();
      this._elem.finalTransform.mProp.applyToMatrix(toWorldMat);
      if (this._elem.hierarchy && this._elem.hierarchy.length) {
        let i;
        const len = this._elem.hierarchy.length;
        for (i = 0; i < len; i += 1) {
          this._elem.hierarchy[i].finalTransform.mProp.applyToMatrix(
              toWorldMat);
        }
        return toWorldMat.inversePoint(arr);
      }
      return toWorldMat.inversePoint(arr);
    }

    function sampleImage() {
      return [1, 1, 1, 1];
    }


    return function(elem) {
      let transformInterface = undefined;

      function _registerMaskInterface(maskManager) {
        _thisLayerFunction.mask = new MaskManagerInterface(maskManager, elem);
      }
      function _registerEffectsInterface(effects) {
        _thisLayerFunction.effect = effects;
      }

      function _thisLayerFunction(name) {
        switch (name) {
          case 'ADBE Root Vectors Group':
          case 'Contents':
          case 2:
            return _thisLayerFunction.shapeInterface;
          case 1:
          case 6:
          case 'Transform':
          case 'transform':
          case 'ADBE Transform Group':
            return transformInterface;
          case 4:
          case 'ADBE Effect Parade':
          case 'effects':
          case 'Effects':
            return _thisLayerFunction.effect;
        }
      }
      _thisLayerFunction.toWorld = toWorld;
      _thisLayerFunction.fromWorld = fromWorld;
      _thisLayerFunction.toComp = toWorld;
      _thisLayerFunction.fromComp = fromComp;
      _thisLayerFunction.sampleImage = sampleImage;
      _thisLayerFunction.sourceRectAtTime = elem.sourceRectAtTime.bind(elem);
      _thisLayerFunction._elem = elem;
      transformInterface =
          TransformExpressionInterface(elem.finalTransform.mProp);
      const anchorPointDescriptor =
          getDescriptor(transformInterface, 'anchorPoint');
      Object.defineProperties(_thisLayerFunction, {
        hasParent: {
          get: function() {
            return elem.hierarchy.length;
          },
        },
        parent: {
          get: function() {
            return elem.hierarchy[0].layerInterface;
          },
        },
        rotation: getDescriptor(transformInterface, 'rotation'),
        scale: getDescriptor(transformInterface, 'scale'),
        position: getDescriptor(transformInterface, 'position'),
        opacity: getDescriptor(transformInterface, 'opacity'),
        anchorPoint: anchorPointDescriptor,
        anchor_point: anchorPointDescriptor,
        transform: {
          get: function() {
            return transformInterface;
          },
        },
        active: {
          get: function() {
            return elem.isInRange;
          },
        },
      });

      _thisLayerFunction.startTime = elem.data.st;
      _thisLayerFunction.index = elem.data.ind;
      _thisLayerFunction.source = elem.data.refId;
      _thisLayerFunction.height = elem.data.ty === 0 ? elem.data.h : 100;
      _thisLayerFunction.width = elem.data.ty === 0 ? elem.data.w : 100;
      _thisLayerFunction.inPoint =
          elem.data.ip / elem.comp.globalData.frameRate;
      _thisLayerFunction.outPoint =
          elem.data.op / elem.comp.globalData.frameRate;
      _thisLayerFunction._name = elem.data.nm;

      _thisLayerFunction.registerMaskInterface = _registerMaskInterface;
      _thisLayerFunction.registerEffectsInterface = _registerEffectsInterface;
      return _thisLayerFunction;
    };
  }());

  const CompExpressionInterface = (function() {
    return function(comp){
        function _thisLayerFunction(name){
          let i = 0;
          const len = comp.layers.length;
          while (i < len) {
            if (comp.layers[i].nm === name || comp.layers[i].ind === name) {
              return comp.elements[i].layerInterface;
            }
            i += 1;
          }
            return null;
            //return {active:false};
        }
        Object.defineProperty(
            _thisLayerFunction, '_name', {value: comp.data.nm});
        _thisLayerFunction.layer = _thisLayerFunction;
        _thisLayerFunction.pixelAspect = 1;
        _thisLayerFunction.height = comp.data.h || comp.globalData.compSize.h;
        _thisLayerFunction.width = comp.data.w || comp.globalData.compSize.w;
        _thisLayerFunction.pixelAspect = 1;
        _thisLayerFunction.frameDuration = 1/comp.globalData.frameRate;
        _thisLayerFunction.displayStartTime = 0;
        _thisLayerFunction.numLayers = comp.layers.length;
        return _thisLayerFunction;
    };
  }());
  const TransformExpressionInterface = (function() {
    return function(transform){
        function _thisFunction(name){
            switch(name){
              case 'scale':
              case 'Scale':
              case 'ADBE Scale':
              case 6:
                return _thisFunction.scale;
              case 'rotation':
              case 'Rotation':
              case 'ADBE Rotation':
              case 'ADBE Rotate Z':
              case 10:
                return _thisFunction.rotation;
              case 'ADBE Rotate X':
                return _thisFunction.xRotation;
              case 'ADBE Rotate Y':
                return _thisFunction.yRotation;
              case 'position':
              case 'Position':
              case 'ADBE Position':
              case 2:
                return _thisFunction.position;
              case 'ADBE Position_0':
                return _thisFunction.xPosition;
              case 'ADBE Position_1':
                return _thisFunction.yPosition;
              case 'ADBE Position_2':
                return _thisFunction.zPosition;
              case 'anchorPoint':
              case 'AnchorPoint':
              case 'Anchor Point':
              case 'ADBE AnchorPoint':
              case 1:
                return _thisFunction.anchorPoint;
              case 'opacity':
              case 'Opacity':
              case 11:
                return _thisFunction.opacity;
            }
        }

        Object.defineProperty(_thisFunction, 'rotation', {
          get: ExpressionPropertyInterface(transform.r || transform.rz),
        });

        Object.defineProperty(_thisFunction, 'zRotation', {
          get: ExpressionPropertyInterface(transform.rz || transform.r),
        });

        Object.defineProperty(_thisFunction, 'xRotation', {
          get: ExpressionPropertyInterface(transform.rx),
        });

        Object.defineProperty(_thisFunction, 'yRotation', {
          get: ExpressionPropertyInterface(transform.ry),
        });
        Object.defineProperty(_thisFunction, 'scale', {
          get: ExpressionPropertyInterface(transform.s),
        });

        const _transformFactory =
            transform.p ? ExpressionPropertyInterface(transform.p) : undefined;
        Object.defineProperty(_thisFunction, 'position', {
          get: function() {
            if (transform.p) {
              return _transformFactory();
            } else {
              return [
                transform.px.v,
                transform.py.v,
                transform.pz ? transform.pz.v : 0,
              ];
            }
          },
        });

        Object.defineProperty(_thisFunction, 'xPosition', {
          get: ExpressionPropertyInterface(transform.px),
        });

        Object.defineProperty(_thisFunction, 'yPosition', {
          get: ExpressionPropertyInterface(transform.py),
        });

        Object.defineProperty(_thisFunction, 'zPosition', {
          get: ExpressionPropertyInterface(transform.pz),
        });

        Object.defineProperty(_thisFunction, 'anchorPoint', {
          get: ExpressionPropertyInterface(transform.a),
        });

        Object.defineProperty(_thisFunction, 'opacity', {
          get: ExpressionPropertyInterface(transform.o),
        });

        Object.defineProperty(_thisFunction, 'skew', {
          get: ExpressionPropertyInterface(transform.sk),
        });

        Object.defineProperty(_thisFunction, 'skewAxis', {
          get: ExpressionPropertyInterface(transform.sa),
        });

        Object.defineProperty(_thisFunction, 'orientation', {
          get: ExpressionPropertyInterface(transform.or),
        });

        return _thisFunction;
    };
  }());
  ProjectInterface = (function() {
    function registerComposition(comp){
      this.compositions.push(comp);
    }

    return function(){
        function _thisProjectFunction(name){
          let i = 0;
          const len = this.compositions.length;
          while (i < len) {
            if (this.compositions[i].data &&
                this.compositions[i].data.nm === name) {
              if (this.compositions[i].prepareFrame &&
                  this.compositions[i].data.xt) {
                this.compositions[i].prepareFrame(this.currentFrame);
              }
              return this.compositions[i].compInterface;
            }
            i += 1;
          }
        }

        _thisProjectFunction.compositions = [];
        _thisProjectFunction.currentFrame = 0;

        _thisProjectFunction.registerComposition = registerComposition;



        return _thisProjectFunction;
    };
  }());
  const EffectsExpressionInterface = (function() {
    const ob = {
      createEffectsInterface: createEffectsInterface,
    };

    function createEffectsInterface(elem, propertyGroup){
      if (elem.effectsManager) {
        const effectElements = [];
        const effectsData = elem.data.ef;
        let i;
        const len = elem.effectsManager.effectElements.length;
        for (i = 0; i < len; i += 1) {
          effectElements.push(createGroupInterface(
              effectsData[i], elem.effectsManager.effectElements[i],
              propertyGroup, elem));
        }

        return function(name) {
          const effects = elem.data.ef || [];
          let i = 0;
          const len = effects.length;
          while (i < len) {
            if (name === effects[i].nm || name === effects[i].mn ||
                name === effects[i].ix) {
              return effectElements[i];
            }
            i += 1;
          }
        };
      }
    }

    function createGroupInterface(data,elements, propertyGroup, elem){
      const effectElements = [];
      let i;
      const len = data.ef.length;
      for (i = 0; i < len; i += 1) {
        if (data.ef[i].ty === 5) {
          effectElements.push(createGroupInterface(
              data.ef[i], elements.effectElements[i],
              elements.effectElements[i].propertyGroup, elem));
        } else {
          effectElements.push(createValueInterface(
              elements.effectElements[i], data.ef[i].ty, elem, _propertyGroup));
        }
      }

      function _propertyGroup(val) {
        if (val === 1) {
          return groupInterface;
        } else {
          return propertyGroup(val - 1);
        }
      }

      const groupInterface = function(name) {
        const effects = data.ef;
        let i = 0;
        const len = effects.length;
        while (i < len) {
          if (name === effects[i].nm || name === effects[i].mn ||
              name === effects[i].ix) {
            if (effects[i].ty === 5) {
              return effectElements[i];
            } else {
              return effectElements[i]();
            }
          }
          i += 1;
        }
        return effectElements[0]();
      };

      groupInterface.propertyGroup = _propertyGroup;

      if (data.mn === 'ADBE Color Control') {
        Object.defineProperty(groupInterface, 'color', {
          get: function() {
            return effectElements[0]();
          },
        });
      }
      Object.defineProperty(groupInterface, 'numProperties', {
        get: function() {
          return data.np;
        },
      });
      groupInterface.active = groupInterface.enabled = data.en !== 0;
      return groupInterface;
    }

    function createValueInterface(element, type, elem, propertyGroup){
      const expressionProperty = ExpressionPropertyInterface(element.p);
      function interfaceFunction() {
        if (type === 10) {
          return elem.comp.compInterface(element.p.v);
        }
        return expressionProperty();
      }

      if (element.p.setGroupProperty) {
        element.p.setGroupProperty(propertyGroup);
      }

      return interfaceFunction;
    }

    return ob;
  }());
  const MaskManagerInterface = (function() {
    function MaskInterface(mask, data) {
      this._mask = mask;
      this._data = data;
    }
    Object.defineProperty(MaskInterface.prototype, 'maskPath', {
      get: function() {
        if (this._mask.prop.k) {
          this._mask.prop.getValue();
        }
        return this._mask.prop;
      },
    });

    const MaskManager = function(maskManager, elem) {
      const _maskManager = maskManager;
      const _elem = elem;
      const _masksInterfaces = createSizedArray(maskManager.viewData.length);
      let i;
      const len = maskManager.viewData.length;
      for (i = 0; i < len; i += 1) {
        _masksInterfaces[i] = new MaskInterface(
            maskManager.viewData[i], maskManager.masksProperties[i]);
      }

      const maskFunction = function(name) {
        i = 0;
        while(i<len){
          if (maskManager.masksProperties[i].nm === name) {
            return _masksInterfaces[i];
          }
          i += 1;
        }
      };
      return maskFunction;
    };
    return MaskManager;
  }());

  const ExpressionPropertyInterface = (function() {
    const defaultUnidimensionalValue = {pv: 0, v: 0, mult: 1};
    const defaultMultidimensionalValue = {pv: [0, 0, 0], v: [0, 0, 0], mult: 1};

    function completeProperty(expressionValue, property, type) {
      Object.defineProperty(expressionValue, 'velocity', {
        get: function() {
          return property.getVelocityAtTime(property.comp.currentFrame);
        },
      });
      expressionValue.numKeys =
          property.keyframes ? property.keyframes.length : 0;
      expressionValue.key = function(pos) {
        if (!expressionValue.numKeys) {
          return 0;
        } else {
          let value = '';
          if ('s' in property.keyframes[pos - 1]) {
            value = property.keyframes[pos - 1].s;
          } else if ('e' in property.keyframes[pos - 2]) {
            value = property.keyframes[pos - 2].e;
          } else {
            value = property.keyframes[pos - 2].s;
          }
          const valueProp = type === 'unidimensional' ?
              new Number(value) :  // eslint-disable-line no-new-wrappers
              Object.assign({}, value);
          valueProp.time = property.keyframes[pos - 1].t /
              property.elem.comp.globalData.frameRate;
          return valueProp;
        }
      };
      expressionValue.valueAtTime = property.getValueAtTime;
      expressionValue.speedAtTime = property.getSpeedAtTime;
      expressionValue.velocityAtTime = property.getVelocityAtTime;
      expressionValue.propertyGroup = property.propertyGroup;
    }

    function UnidimensionalPropertyInterface(property) {
      if (!property || !('pv' in property)) {
        property = defaultUnidimensionalValue;
      }
      const mult = 1 / property.mult;
      let val = property.pv * mult;
      // eslint-disable-next-line no-new-wrappers
      let expressionValue = new Number(val);
      expressionValue.value = val;
      completeProperty(expressionValue, property, 'unidimensional');

      return function() {
        if (property.k) {
          property.getValue();
        }
        val = property.v * mult;
        if (expressionValue.value !== val) {
          // eslint-disable-next-line no-new-wrappers
          expressionValue = new Number(val);
          expressionValue.value = val;
          completeProperty(expressionValue, property, 'unidimensional');
        }
        return expressionValue;
      };
    }

    function MultidimensionalPropertyInterface(property) {
      if (!property || !('pv' in property)) {
        property = defaultMultidimensionalValue;
      }
      const mult = 1 / property.mult;
      const len = property.pv.length;
      const expressionValue = createTypedArray('float32', len);
      const arrValue = createTypedArray('float32', len);
      expressionValue.value = arrValue;
      completeProperty(expressionValue, property, 'multidimensional');

      return function() {
        if (property.k) {
          property.getValue();
        }
        for (let i = 0; i < len; i += 1) {
          expressionValue[i] = arrValue[i] = property.v[i] * mult;
        }
        return expressionValue;
      };
    }

    //TODO: try to avoid using this getter
    function defaultGetter() {
      return defaultUnidimensionalValue;
    }

    return function(property) {
      if (!property) {
        return defaultGetter;
      } else if (property.propType === 'unidimensional') {
        return UnidimensionalPropertyInterface(property);
      } else {
        return MultidimensionalPropertyInterface(property);
      }
    };
  }());

  (function() {
    const TextExpressionSelectorProp = (function() {
      function getValueProxy(index, total) {
        this.textIndex = index + 1;
        this.textTotal = total;
        this.v = this.getValue() * this.mult;
        return this.v;
      }

      return function TextExpressionSelectorProp(elem, data) {
        this.pv = 1;
        this.comp = elem.comp;
        this.elem = elem;
        this.mult = 0.01;
        this.propType = 'textSelector';
        this.textTotal = data.totalChars;
        this.selectorValue = 100;
        this.lastValue = [1, 1, 1];
        this.k = true;
        this.x = true;
        this.getValue =
            ExpressionManager.initiateExpression.bind(this)(elem, data, this);
        this.getMult = getValueProxy;
        this.getVelocityAtTime = expressionHelpers.getVelocityAtTime;
        if (this.kf) {
          this.getValueAtTime = expressionHelpers.getValueAtTime.bind(this);
        } else {
          this.getValueAtTime =
              expressionHelpers.getStaticValueAtTime.bind(this);
        }
        this.setGroupProperty = expressionHelpers.setGroupProperty;
      };
    }());

    const propertyGetTextProp = TextSelectorProp.getTextSelectorProp;
    TextSelectorProp.getTextSelectorProp = function(elem, data, arr) {
      if(data.t === 1){
        return new TextExpressionSelectorProp(elem, data, arr);
      } else {
        return propertyGetTextProp(elem, data, arr);
      }
    };
  }());
  function SliderEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,0,0,container);
  }
  function AngleEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,0,0,container);
  }
  function ColorEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,1,0,container);
  }
  function PointEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,1,0,container);
  }
  function LayerIndexEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,0,0,container);
  }
  function MaskIndexEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,0,0,container);
  }
  function CheckboxEffect(data, elem, container) {
    this.p = PropertyFactory.getProp(elem,data.v,0,0,container);
  }
  function NoValueEffect() {
    this.p = {};
  }
  function EffectsManager(data, element) {
    const effects = data.ef || [];
    this.effectElements = [];
    let i;
    const len = effects.length;
    let effectItem;
    for(i=0;i<len;i++) {
      effectItem = new GroupEffect(effects[i], element);
      this.effectElements.push(effectItem);
    }
  }

  function GroupEffect(data, element) {
    this.init(data,element);
  }

  extendPrototype([DynamicPropertyContainer], GroupEffect);

  GroupEffect.prototype.getValue =
      GroupEffect.prototype.iterateDynamicProperties;

  GroupEffect.prototype.init = function(data, element) {
    this.data = data;
    this.effectElements = [];
    this.initDynamicPropertyContainer(element);
    let i;
    const len = this.data.ef.length;
    let eff;
    const effects = this.data.ef;
    for(i=0;i<len;i+=1){
      eff = null;
      switch (effects[i].ty) {
        case 0:
          eff = new SliderEffect(effects[i], element, this);
          break;
        case 1:
          eff = new AngleEffect(effects[i], element, this);
          break;
        case 2:
          eff = new ColorEffect(effects[i], element, this);
          break;
        case 3:
          eff = new PointEffect(effects[i], element, this);
          break;
        case 4:
        case 7:
          eff = new CheckboxEffect(effects[i], element, this);
          break;
        case 10:
          eff = new LayerIndexEffect(effects[i], element, this);
          break;
        case 11:
          eff = new MaskIndexEffect(effects[i], element, this);
          break;
        case 5:
          eff = new EffectsManager(effects[i], element, this);
          break;
        // case 6:
        default:
          eff = new NoValueEffect(effects[i], element, this);
          break;
      }
      if (eff) {
        this.effectElements.push(eff);
      }
    }
  };

  const lottiejs = {};

  const _isFrozen = false;

  function loadAnimation(params) {
    return animationManager.loadAnimation(params);
  }

  function setQuality(value) {
    if (typeof value === 'string') {
      switch (value) {
        case 'high':
          defaultCurveSegments = 200;
          break;
        case 'medium':
          defaultCurveSegments = 50;
          break;
        case 'low':
          defaultCurveSegments = 10;
          break;
      }
    } else if (!isNaN(value) && value > 1) {
      defaultCurveSegments = value;
    }
    if (defaultCurveSegments >= 50) {
      roundValues(false);
    } else {
      roundValues(true);
    }
  }

  lottiejs.play = animationManager.play;
  lottiejs.pause = animationManager.pause;
  lottiejs.togglePause = animationManager.togglePause;
  lottiejs.setSpeed = animationManager.setSpeed;
  lottiejs.setDirection = animationManager.setDirection;
  lottiejs.stop = animationManager.stop;
  lottiejs.registerAnimation = animationManager.registerAnimation;
  lottiejs.loadAnimation = loadAnimation;
  lottiejs.resize = animationManager.resize;
  lottiejs.goToAndStop = animationManager.goToAndStop;
  lottiejs.destroy = animationManager.destroy;
  lottiejs.setQuality = setQuality;
  lottiejs.freeze = animationManager.freeze;
  lottiejs.unfreeze = animationManager.unfreeze;
  lottiejs.getRegisteredAnimations = animationManager.getRegisteredAnimations;
  lottiejs.version = '5.5.2';

  const renderer = '';
  return lottiejs;
})({});


/**
 * Code below is added to handle the messages sent to the worker thread. Its a
 * a wrapper that manages animation control for each animation.
 */

/**
 * An instance of the currently running lottie animation.
 * @type {?AnimationItem}
 */
let currentAnimation = null;

/**
 * Events sent back to the parent thread.
 */
const events = {
  INITIALIZED: 'initialized',  // Send when the animation was successfully
                               // initialized.
  RESIZED: 'resized',          // Sent when the animation has been resized.
  PLAYING: 'playing',          // Send when the animation started playing.
  PAUSED: 'paused',            // Sent when the animation has paused.
  STOPPED: 'stopped',          // Sent when the animation has stopped.
};

/**
 * Returns the size of the canvas on which the current animation is running on.
 * @return {Object<string, number>} Returns the current size of canvas
 */
function getCurrentCanvasSize() {
  const canvas = currentAnimation.renderer.canvasContext.canvas;
  return {
    height: canvas.height,
    width: canvas.width,
  };
}

/**
 * Informs the parent thread that the canvas has been resized and also sends the
 * new size.
 */
function sendResizeEvent() {
  const canvas = currentAnimation.renderer.canvasContext.canvas;
  postMessage({
    name: events.RESIZED,
    size: getCurrentCanvasSize(),
  });
}

/**
 * Informs the parent thread that the animation is playing.
 */
function sendPlayEvent() {
  postMessage({name: events.PLAYING});
}

/**
 * Informs the parent thread that the animation is paused.
 */
function sendPauseEvent() {
  postMessage({name: events.PAUSED});
}

/**
 * Informs the parent thread that the animation is stopped.
 */
function sendStopEvent() {
  postMessage({name: events.STOPPED});
}

/**
 * Informs the parent thread that the animation has finished initializing.
 */
function sendInitializedEvent() {
  const canvas = currentAnimation.renderer.canvasContext.canvas;
  postMessage({
    name: events.INITIALIZED,
    success: currentAnimation.isLoaded,
  });
}

/**
 * Initializes the animation using the animation data sent from the parent
 * thread along with the init parameters. If an animation is already initialized
 * this is a no-op.
 * @param {JSON} animationData JSON data for the animation.
 * @param {Object} initParams Parameters to initialize the animation with.
 * @param {OffscreenCanvas} canvas The offsccreen canvas that will display the
 *     animation.
 */
function initAnimation(animationData, initParams, canvas) {
  if (!animationData || !initParams) {
    return;
  }

  const ctx = canvas.getContext('2d');

  currentAnimation = lottiejs.loadAnimation({
    renderer: 'canvas',
    loop: initParams.loop,
    autoplay: initParams.autoplay,
    animationData: animationData,
    rendererSettings: {
      context: ctx,
      scaleMode: 'noScale',
      clearCanvas: true,
    },
  });

  sendInitializedEvent();

  // Play the animation if its not already playing.
  if (initParams.autoplay) {
    currentAnimation.play();
  }

  if (currentAnimation.isLoaded && !currentAnimation.isPaused) {
    sendPlayEvent();
  }
}

/**
 * Updates the canvas draw size. If an animation is already initialized or
 * playing, this would update the canvas for that as well.
 * @param {OffscreenCanvas} canvas Draw size for this canvas is updated.
 * @param {Object<string, number>} size Draw size to update the canvas to.
 */
function updateCanvasSize(canvas, size) {
  if (!size || !canvas) {
    return;
  }

  if (size.height > 0 && size.width > 0) {
    canvas.height = size.height;
    canvas.width = size.width;
  }

  // If an animation is already initialized then update its canvas size.
  if (currentAnimation) {
    currentAnimation.resize();
    sendResizeEvent();
  }
}

/**
 * Updates the animation state to play or pause. Informs the parent thread if
 * the state has changed.
 * @param {Object<string, bool>} control Has information about playing or
 *     pausing the animation.
 * @param {HTMLCanvasElement} canvas the canvas on which the animation is drawn.
 */
function updateAnimationState(control, canvas) {
  if (!control || !currentAnimation) {
    return;
  }

  if (control.stop) {
    currentAnimation.stop();
    canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
    sendStopEvent();
    return;
  }

  if (control.play && currentAnimation.isPaused) {
    currentAnimation.play();
    sendPlayEvent();
  } else if (!control.play && !currentAnimation.isPaused) {
    currentAnimation.pause();
    sendPauseEvent();
  }
}

onmessage = function(evt) {
  if (!evt || !evt.data) return;

  let canvas = null;
  if (currentAnimation) {
    canvas = currentAnimation.renderer.canvasContext.canvas;
  } else if (evt.data.canvas) {
    canvas = evt.data.canvas;
  } else {
    return;
  }

  // Stop and clear the current animation to initialize a new one with the
  // provided animation data.
  if (currentAnimation && evt.data.animationData) {
    currentAnimation.stop();
    currentAnimation = null;
  }

  updateCanvasSize(canvas, evt.data.drawSize);
  initAnimation(evt.data.animationData, evt.data.params, canvas);
  updateAnimationState(evt.data.control, canvas);
};
