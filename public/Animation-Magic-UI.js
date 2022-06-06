var statusElement = document.getElementById("status"),
  progressElement = document.getElementById("progress"),
  spinnerElement = document.getElementById("spinner"),
  Module = {
    noImageDecoding: true,
    noAudioDecoding: true,
    preRun: [],
    postRun: [],
    print: (function () {
      var e = document.getElementById("output");
      return (
        e && (e.value = ""),
        function (t) {
          arguments.length > 1 &&
            (t = Array.prototype.slice.call(arguments).join(" ")),
            console.log(t),
            e && ((e.value += t + "\n"), (e.scrollTop = e.scrollHeight));
        }
      );
    })(),
    printErr: function (e) {
      arguments.length > 1 &&
        (e = Array.prototype.slice.call(arguments).join(" ")),
        console.error(e);
    },
    canvas: (function () {
      var e = document.getElementById("canvas");
      return (
        e.addEventListener(
          "webglcontextlost",
          function (e) {
            alert("WebGL context lost. You will need to reload the page."),
              e.preventDefault();
          },
          !1
        ),
        e
      );
    })(),
    setStatus: function (e) {
      if (
        (Module.setStatus.last ||
          (Module.setStatus.last = { time: Date.now(), text: "" }),
        e !== Module.setStatus.last.text)
      ) {
        var t = e.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/),
          n = Date.now();
        (t && n - Module.setStatus.last.time < 30) ||
          ((Module.setStatus.last.time = n),
          (Module.setStatus.last.text = e),
          t
            ? ((e = t[1]),
              (progressElement.value = 100 * parseInt(t[2])),
              (progressElement.max = 100 * parseInt(t[4])),
              (progressElement.hidden = !1),
              (spinnerElement.hidden = !1))
            : ((progressElement.value = null),
              (progressElement.max = null),
              (progressElement.hidden = !0),
              e || (spinnerElement.style.display = "none")),
          (statusElement.innerHTML = e));
      }
    },
    totalDependencies: 0,
    monitorRunDependencies: function (e) {
      (this.totalDependencies = Math.max(this.totalDependencies, e)),
        Module.setStatus(
          e
            ? "Preparing... (" +
                (this.totalDependencies - e) +
                "/" +
                this.totalDependencies +
                ")"
            : "All downloads complete."
        );
    },
  };
Module.setStatus("Downloading..."),
  (window.onerror = function (e) {
    Module.setStatus("Exception thrown, see JavaScript console"),
      (spinnerElement.style.display = "none"),
      (Module.setStatus = function (e) {
        e && Module.printErr("[post-exception status] " + e);
      });
  });

function resizeCanvas() {
    var canvas = document.getElementById("canvas");
    canvas.style.width = window.innerWidth + 'px';
    canvas.style.height = window.innerHeight + 'px';
};

function resizeFramebuffers() {
    _updateWindowDimensions(window.innerWidth, window.innerHeight);
};

window.addEventListener('resize', resizeCanvas, false);

window.addEventListener('load', () => {
    resizeCanvas();

    window.addEventListener('resize', resizeFramebuffers, false);

    const gl = document.createElement('canvas').getContext('webgl2');
    if (!gl) {
      document.getElementById('webgl_unsupported_popup').style.display = 'block';
    }
    else {
      document.getElementById('learn_more_button').style.display = 'block';
    }
});

// Get the modal
var modal = document.getElementById("myModal");

// Get the button that opens the modal
var btn = document.getElementById("learn_more_button");

// Get the <span> element that closes the modal
var span = document.getElementsByClassName("close")[0];

// When the user clicks on the button, open the modal
btn.onclick = function() {
  modal.style.display = "block";
}

// When the user clicks on <span> (x), close the modal
span.onclick = function() {
  modal.style.display = "none";
}
