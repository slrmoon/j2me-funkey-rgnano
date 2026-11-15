
  var Module = typeof Module !== 'undefined' ? Module : {};

  if (!Module.expectedDataFileDownloads) {
    Module.expectedDataFileDownloads = 0;
  }

  Module.expectedDataFileDownloads++;
  (function() {
    // When running as a pthread, FS operations are proxied to the main thread, so we don't need to
    // fetch the .data bundle on the worker
    if (Module['ENVIRONMENT_IS_PTHREAD']) return;
    var loadPackage = function(metadata) {

      var PACKAGE_PATH = '';
      if (typeof window === 'object') {
        PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.toString().substring(0, window.location.pathname.toString().lastIndexOf('/')) + '/');
      } else if (typeof process === 'undefined' && typeof location !== 'undefined') {
        // web worker
        PACKAGE_PATH = encodeURIComponent(location.pathname.toString().substring(0, location.pathname.toString().lastIndexOf('/')) + '/');
      }
      var PACKAGE_NAME = '/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data';
      var REMOTE_PACKAGE_BASE = 'midp-files.data';
      if (typeof Module['locateFilePackage'] === 'function' && !Module['locateFile']) {
        Module['locateFile'] = Module['locateFilePackage'];
        err('warning: you defined Module.locateFilePackage, that has been renamed to Module.locateFile (using your locateFilePackage for now)');
      }
      var REMOTE_PACKAGE_NAME = Module['locateFile'] ? Module['locateFile'](REMOTE_PACKAGE_BASE, '') : REMOTE_PACKAGE_BASE;

      var REMOTE_PACKAGE_SIZE = metadata['remote_package_size'];
      var PACKAGE_UUID = metadata['package_uuid'];

      function fetchRemotePackage(packageName, packageSize, callback, errback) {
        if (typeof process === 'object' && typeof process.versions === 'object' && typeof process.versions.node === 'string') {
          require('fs').readFile(packageName, function(err, contents) {
            if (err) {
              errback(err);
            } else {
              callback(contents.buffer);
            }
          });
          return;
        }
        var xhr = new XMLHttpRequest();
        xhr.open('GET', packageName, true);
        xhr.responseType = 'arraybuffer';
        xhr.onprogress = function(event) {
          var url = packageName;
          var size = packageSize;
          if (event.total) size = event.total;
          if (event.loaded) {
            if (!xhr.addedTotal) {
              xhr.addedTotal = true;
              if (!Module.dataFileDownloads) Module.dataFileDownloads = {};
              Module.dataFileDownloads[url] = {
                loaded: event.loaded,
                total: size
              };
            } else {
              Module.dataFileDownloads[url].loaded = event.loaded;
            }
            var total = 0;
            var loaded = 0;
            var num = 0;
            for (var download in Module.dataFileDownloads) {
            var data = Module.dataFileDownloads[download];
              total += data.total;
              loaded += data.loaded;
              num++;
            }
            total = Math.ceil(total * Module.expectedDataFileDownloads/num);
            if (Module['setStatus']) Module['setStatus']('Downloading data... (' + loaded + '/' + total + ')');
          } else if (!Module.dataFileDownloads) {
            if (Module['setStatus']) Module['setStatus']('Downloading data...');
          }
        };
        xhr.onerror = function(event) {
          throw new Error("NetworkError for: " + packageName);
        }
        xhr.onload = function(event) {
          if (xhr.status == 200 || xhr.status == 304 || xhr.status == 206 || (xhr.status == 0 && xhr.response)) { // file URLs can return 0
            var packageData = xhr.response;
            callback(packageData);
          } else {
            throw new Error(xhr.statusText + " : " + xhr.responseURL);
          }
        };
        xhr.send(null);
      };

      function handleError(error) {
        console.error('package error:', error);
      };

      var fetchedCallback = null;
      var fetched = Module['getPreloadedPackage'] ? Module['getPreloadedPackage'](REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE) : null;

      if (!fetched) fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE, function(data) {
        if (fetchedCallback) {
          fetchedCallback(data);
          fetchedCallback = null;
        } else {
          fetched = data;
        }
      }, handleError);

    function runWithFS() {

      function assert(check, msg) {
        if (!check) throw msg + new Error().stack;
      }
Module['FS_createPath']("/", "midp", true, true);
Module['FS_createPath']("/midp", "lib", true, true);
Module['FS_createPath']("/midp", "appdb", true, true);

      /** @constructor */
      function DataRequest(start, end, audio) {
        this.start = start;
        this.end = end;
        this.audio = audio;
      }
      DataRequest.prototype = {
        requests: {},
        open: function(mode, name) {
          this.name = name;
          this.requests[name] = this;
          Module['addRunDependency']('fp ' + this.name);
        },
        send: function() {},
        onload: function() {
          var byteArray = this.byteArray.subarray(this.start, this.end);
          this.finish(byteArray);
        },
        finish: function(byteArray) {
          var that = this;
          // canOwn this data in the filesystem, it is a slide into the heap that will never change
          Module['FS_createDataFile'](this.name, null, byteArray, true, true, true);
          Module['removeRunDependency']('fp ' + that.name);
          this.requests[this.name] = null;
        }
      };

      var files = metadata['files'];
      for (var i = 0; i < files.length; ++i) {
        new DataRequest(files[i]['start'], files[i]['end'], files[i]['audio'] || 0).open('GET', files[i]['filename']);
      }

      function processPackageData(arrayBuffer) {
        assert(arrayBuffer, 'Loading data file failed.');
        assert(arrayBuffer instanceof ArrayBuffer, 'bad input to processPackageData');
        var byteArray = new Uint8Array(arrayBuffer);
        var curr;
        // Reuse the bytearray from the XHR as the source for file reads.
          DataRequest.prototype.byteArray = byteArray;
          var files = metadata['files'];
          for (var i = 0; i < files.length; ++i) {
            DataRequest.prototype.requests[files[i].filename].onload();
          }          Module['removeRunDependency']('datafile_/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data');

      };
      Module['addRunDependency']('datafile_/home/runner/work/j2me-funkey-rgnano/j2me-funkey-rgnano/out/web/site/midp-files.data');

      if (!Module.preloadResults) Module.preloadResults = {};

      Module.preloadResults[PACKAGE_NAME] = {fromCache: false};
      if (fetched) {
        processPackageData(fetched);
        fetched = null;
      } else {
        fetchedCallback = processPackageData;
      }

    }
    if (Module['calledRun']) {
      runWithFS();
    } else {
      if (!Module['preRun']) Module['preRun'] = [];
      Module["preRun"].push(runWithFS); // FS is not initialized yet, wait for it
    }

    }
    loadPackage({"files": [{"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 0, "end": 1900}, {"filename": "/midp/lib/screen.image_bg.png", "start": 1900, "end": 2048}, {"filename": "/midp/lib/title.image_bg1.png", "start": 2048, "end": 2229}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 2229, "end": 2539}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 2539, "end": 2702}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 2702, "end": 3438}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 3438, "end": 3661}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 3661, "end": 5940}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 5940, "end": 7313}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 7313, "end": 7533}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 7533, "end": 7867}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 7867, "end": 8027}, {"filename": "/midp/lib/scroll.image_up.png", "start": 8027, "end": 8265}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 8265, "end": 8472}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 8472, "end": 8649}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 8649, "end": 10036}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 10036, "end": 10769}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 10769, "end": 10989}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 10989, "end": 11142}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 11142, "end": 11396}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 11396, "end": 11549}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 11549, "end": 14190}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 14190, "end": 14413}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 14413, "end": 14630}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 14630, "end": 16717}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 16717, "end": 17666}, {"filename": "/midp/lib/untrusted_icon.png", "start": 17666, "end": 18695}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 18695, "end": 23576}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 23576, "end": 23946}, {"filename": "/midp/lib/title.image_bg2.png", "start": 23946, "end": 24546}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 24546, "end": 26711}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 26711, "end": 28684}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 28684, "end": 28895}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 28895, "end": 29073}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 29073, "end": 30810}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 30810, "end": 31146}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 31146, "end": 32469}, {"filename": "/midp/lib/pbar.image_values.png", "start": 32469, "end": 34362}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 34362, "end": 35443}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 35443, "end": 36170}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 36170, "end": 37022}, {"filename": "/midp/lib/title.image_bg0.png", "start": 37022, "end": 37865}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 37865, "end": 38159}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 38159, "end": 38360}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 38360, "end": 40578}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 40578, "end": 40810}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 40810, "end": 40996}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 40996, "end": 42981}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 42981, "end": 43141}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 43141, "end": 44935}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 44935, "end": 45229}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 45229, "end": 46940}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 46940, "end": 47110}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 47110, "end": 47270}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 47270, "end": 47489}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 47489, "end": 48414}, {"filename": "/midp/lib/doja-support.jar", "start": 48414, "end": 909055}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 909055, "end": 909244}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 909244, "end": 909557}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 909557, "end": 909768}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 909768, "end": 910057}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 910057, "end": 910247}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 910247, "end": 911144}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 911144, "end": 911333}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 911333, "end": 911498}, {"filename": "/midp/lib/indicator_network.png", "start": 911498, "end": 912018}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 912018, "end": 912178}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 912178, "end": 912514}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 912514, "end": 912674}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 912674, "end": 912906}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 912906, "end": 913081}, {"filename": "/midp/lib/indicator_home.png", "start": 913081, "end": 913690}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 913690, "end": 915251}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 915251, "end": 915454}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 915454, "end": 915641}, {"filename": "/midp/lib/trusted_icon.png", "start": 915641, "end": 916433}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 916433, "end": 916640}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 916640, "end": 916853}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 916853, "end": 917039}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 917039, "end": 917340}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 917340, "end": 919297}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 919297, "end": 920733}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 920733, "end": 921034}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 921034, "end": 921803}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 921803, "end": 922026}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 922026, "end": 922176}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 922176, "end": 922346}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 922346, "end": 922832}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 922832, "end": 922985}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 922985, "end": 923934}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 923934, "end": 925424}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 925424, "end": 925713}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 925713, "end": 926552}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 926552, "end": 926715}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 926715, "end": 928498}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 928498, "end": 929494}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 929494, "end": 933138}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 933138, "end": 933309}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 933309, "end": 933510}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 933510, "end": 933666}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 933666, "end": 933835}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 933835, "end": 934148}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 934148, "end": 934322}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 934322, "end": 934873}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 934873, "end": 935061}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 935061, "end": 935293}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 935293, "end": 935525}, {"filename": "/midp/lib/calendar.png", "start": 935525, "end": 935656}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 935656, "end": 936140}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 936140, "end": 936359}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 936359, "end": 938114}, {"filename": "/midp/lib/indicator_bar.png", "start": 938114, "end": 940059}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 940059, "end": 942589}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 942589, "end": 942756}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 942756, "end": 943011}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 943011, "end": 943170}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 943170, "end": 945465}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 945465, "end": 946218}, {"filename": "/midp/lib/gauge.image_values.png", "start": 946218, "end": 947357}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 947357, "end": 947522}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 947522, "end": 949217}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 949217, "end": 949404}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 949404, "end": 951779}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 951779, "end": 953691}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 953691, "end": 954363}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 954363, "end": 954590}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 954590, "end": 955781}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 955781, "end": 956694}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 956694, "end": 958476}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 958476, "end": 960205}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 960205, "end": 962532}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 962532, "end": 962755}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 962755, "end": 962941}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 962941, "end": 963650}, {"filename": "/midp/lib/skin.bin", "start": 963650, "end": 969426}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 969426, "end": 969593}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 969593, "end": 970888}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 970888, "end": 971125}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 971125, "end": 971504}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 971504, "end": 971754}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 971754, "end": 971955}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 971955, "end": 973727}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 973727, "end": 975631}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 975631, "end": 976051}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 976051, "end": 977835}, {"filename": "/midp/lib/indicator_trusted.png", "start": 977835, "end": 978682}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 978682, "end": 979349}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 979349, "end": 979581}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 979581, "end": 980584}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 980584, "end": 980758}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 980758, "end": 982542}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 982542, "end": 984286}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 984286, "end": 984554}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 984554, "end": 985227}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 985227, "end": 986738}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 986738, "end": 987557}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 987557, "end": 987764}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 987764, "end": 988061}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 988061, "end": 989021}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 989021, "end": 989198}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 989198, "end": 989363}, {"filename": "/midp/lib/screen.image_wash.png", "start": 989363, "end": 989525}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 989525, "end": 989698}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 989698, "end": 989930}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 989930, "end": 990091}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 990091, "end": 990272}, {"filename": "/midp/appdb/_suite8.png", "start": 990272, "end": 991064}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 991064, "end": 991784}, {"filename": "/midp/appdb/_single8.png", "start": 991784, "end": 992582}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 992582, "end": 995918}, {"filename": "/midp/appdb/_main.ks", "start": 995918, "end": 996891}, {"filename": "/midp/appdb/_dukeok8.png", "start": 996891, "end": 1000915}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 1000915, "end": 1009640}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1009640, "end": 1012604}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1012604, "end": 1015906}, {"filename": "/midp/appdb/_ch_single.png", "start": 1015906, "end": 1019121}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 1019121, "end": 1027885}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1027885, "end": 1028950}, {"filename": "/midp/appdb/_ack8.png", "start": 1028950, "end": 1032683}], "remote_package_size": 1032683, "package_uuid": "948e09e0-780a-44d8-97cb-6a9c17b74f60"});

  })();
