
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
      var PACKAGE_NAME = '/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data';
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
          }          Module['removeRunDependency']('datafile_/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data');

      };
      Module['addRunDependency']('datafile_/home/claude/j2me-funkey-rgnano/out/web/site/midp-files.data');

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
    loadPackage({"files": [{"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 0, "end": 150}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 150, "end": 1341}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 1341, "end": 1515}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 1515, "end": 1716}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 1716, "end": 1876}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 1876, "end": 2099}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 2099, "end": 2302}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 2302, "end": 2615}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 2615, "end": 2838}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 2838, "end": 3007}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 3007, "end": 4443}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 4443, "end": 4603}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 4603, "end": 4778}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 4778, "end": 5450}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 5450, "end": 5682}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 5682, "end": 7069}, {"filename": "/midp/lib/pbar.image_values.png", "start": 7069, "end": 8962}, {"filename": "/midp/lib/indicator_network.png", "start": 8962, "end": 9482}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 9482, "end": 9689}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 9689, "end": 9983}, {"filename": "/midp/lib/skin.bin", "start": 9983, "end": 15759}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 15759, "end": 16060}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 16060, "end": 17056}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 17056, "end": 17353}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 17353, "end": 17516}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 17516, "end": 17681}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 17681, "end": 17913}, {"filename": "/midp/lib/indicator_bar.png", "start": 17913, "end": 19858}, {"filename": "/midp/lib/gauge.image_values.png", "start": 19858, "end": 20997}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 20997, "end": 21214}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 21214, "end": 22943}, {"filename": "/midp/lib/calendar.png", "start": 22943, "end": 23074}, {"filename": "/midp/lib/title.image_bg0.png", "start": 23074, "end": 23917}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 23917, "end": 25407}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 25407, "end": 26140}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 26140, "end": 27065}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 27065, "end": 29050}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 29050, "end": 29210}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 29210, "end": 29696}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 29696, "end": 30515}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 30515, "end": 30765}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 30765, "end": 30951}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 30951, "end": 31183}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 31183, "end": 33824}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 33824, "end": 34118}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 34118, "end": 34845}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 34845, "end": 35077}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 35077, "end": 35345}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 35345, "end": 35715}, {"filename": "/midp/lib/trusted_icon.png", "start": 35715, "end": 36507}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 36507, "end": 36667}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 36667, "end": 37376}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 37376, "end": 38671}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 38671, "end": 40382}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 40382, "end": 42757}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 42757, "end": 43091}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 43091, "end": 43279}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 43279, "end": 44131}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 44131, "end": 44444}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 44444, "end": 44864}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 44864, "end": 47143}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 47143, "end": 47344}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 47344, "end": 47564}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 47564, "end": 47783}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 47783, "end": 48622}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 48622, "end": 51152}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 51152, "end": 51441}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 51441, "end": 52522}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 52522, "end": 52678}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 52678, "end": 53627}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 53627, "end": 53804}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 53804, "end": 53969}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 53969, "end": 54180}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 54180, "end": 55917}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 55917, "end": 58135}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 58135, "end": 58342}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 58342, "end": 58495}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 58495, "end": 59231}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 59231, "end": 61014}, {"filename": "/midp/lib/indicator_home.png", "start": 61014, "end": 61623}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 61623, "end": 61855}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 61855, "end": 62109}, {"filename": "/midp/lib/indicator_trusted.png", "start": 62109, "end": 62956}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 62956, "end": 63507}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 63507, "end": 65411}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 65411, "end": 65634}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 65634, "end": 65847}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 65847, "end": 66006}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 66006, "end": 66184}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 66184, "end": 66403}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 66403, "end": 66604}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 66604, "end": 66815}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 66815, "end": 67568}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 67568, "end": 69895}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 69895, "end": 71868}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 71868, "end": 72035}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 72035, "end": 72188}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 72188, "end": 72489}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 72489, "end": 72868}, {"filename": "/midp/lib/untrusted_icon.png", "start": 72868, "end": 73897}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 73897, "end": 74900}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 74900, "end": 75669}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 75669, "end": 77230}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 77230, "end": 77390}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 77390, "end": 77576}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 77576, "end": 79331}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 79331, "end": 81418}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 81418, "end": 82315}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 82315, "end": 82482}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 82482, "end": 82771}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 82771, "end": 84466}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 84466, "end": 84686}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 84686, "end": 86981}, {"filename": "/midp/lib/title.image_bg1.png", "start": 86981, "end": 87162}, {"filename": "/midp/lib/screen.image_bg.png", "start": 87162, "end": 87310}, {"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 87310, "end": 89210}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 89210, "end": 94091}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 94091, "end": 95873}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 95873, "end": 96046}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 96046, "end": 98003}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 98003, "end": 99787}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 99787, "end": 100019}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 100019, "end": 101931}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 101931, "end": 102186}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 102186, "end": 103559}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 103559, "end": 105353}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 105353, "end": 105542}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 105542, "end": 105765}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 105765, "end": 106678}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 106678, "end": 107014}, {"filename": "/midp/lib/screen.image_wash.png", "start": 107014, "end": 107176}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 107176, "end": 107843}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 107843, "end": 111487}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 111487, "end": 111657}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 111657, "end": 111828}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 111828, "end": 112788}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 112788, "end": 112965}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 112965, "end": 113118}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 113118, "end": 115283}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 115283, "end": 115469}, {"filename": "/midp/lib/scroll.image_up.png", "start": 115469, "end": 115707}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 115707, "end": 115934}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 115934, "end": 116115}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 116115, "end": 116304}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 116304, "end": 116465}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 116465, "end": 116630}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 116630, "end": 117953}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 117953, "end": 118123}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 118123, "end": 118459}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 118459, "end": 119132}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 119132, "end": 120904}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 120904, "end": 122688}, {"filename": "/midp/lib/title.image_bg2.png", "start": 122688, "end": 123288}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 123288, "end": 123495}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 123495, "end": 123805}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 123805, "end": 124042}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 124042, "end": 125553}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 125553, "end": 125740}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 125740, "end": 125903}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 125903, "end": 127647}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 127647, "end": 127821}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 127821, "end": 128008}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 128008, "end": 128198}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 128198, "end": 129147}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 129147, "end": 132111}, {"filename": "/midp/appdb/_single8.png", "start": 132111, "end": 132909}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 132909, "end": 133629}, {"filename": "/midp/appdb/_ch_suite.png", "start": 133629, "end": 136931}, {"filename": "/midp/appdb/_main.ks", "start": 136931, "end": 137904}, {"filename": "/midp/appdb/_suite8.png", "start": 137904, "end": 138696}, {"filename": "/midp/appdb/_dukeok8.png", "start": 138696, "end": 142720}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 142720, "end": 151445}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 151445, "end": 154781}, {"filename": "/midp/appdb/_ch_single.png", "start": 154781, "end": 157996}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 157996, "end": 159061}, {"filename": "/midp/appdb/_ack8.png", "start": 159061, "end": 162794}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 162794, "end": 171558}], "remote_package_size": 171558, "package_uuid": "b1ff8808-82ad-42e2-8f55-dc75cadccff2"});

  })();
