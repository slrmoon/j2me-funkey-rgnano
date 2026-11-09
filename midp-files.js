
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
    loadPackage({"files": [{"filename": "/midp/lib/imageitem.image_btn2.png", "start": 0, "end": 313}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 313, "end": 5194}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 5194, "end": 5381}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 5381, "end": 7366}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 7366, "end": 9323}, {"filename": "/midp/lib/screen.image_bg.png", "start": 9323, "end": 9471}, {"filename": "/midp/lib/title.image_bg2.png", "start": 9471, "end": 10071}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 10071, "end": 10231}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 10231, "end": 12558}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 12558, "end": 12813}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 12813, "end": 12991}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 12991, "end": 15209}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 15209, "end": 15942}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 15942, "end": 16891}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 16891, "end": 17080}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 17080, "end": 17247}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 17247, "end": 17412}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 17412, "end": 18848}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 18848, "end": 19080}, {"filename": "/midp/lib/gauge.image_values.png", "start": 19080, "end": 20219}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 20219, "end": 22003}, {"filename": "/midp/lib/indicator_network.png", "start": 22003, "end": 22523}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 22523, "end": 22812}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 22812, "end": 22972}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 22972, "end": 25251}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 25251, "end": 25483}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 25483, "end": 25646}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 25646, "end": 27811}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 27811, "end": 29784}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 29784, "end": 29954}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 29954, "end": 31737}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 31737, "end": 31890}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 31890, "end": 32050}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 32050, "end": 32273}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 32273, "end": 32423}, {"filename": "/midp/lib/untrusted_icon.png", "start": 32423, "end": 33452}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 33452, "end": 34825}, {"filename": "/midp/lib/indicator_trusted.png", "start": 34825, "end": 35672}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 35672, "end": 37162}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 37162, "end": 39803}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 39803, "end": 40556}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 40556, "end": 44200}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 44200, "end": 44386}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 44386, "end": 44551}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 44551, "end": 44801}, {"filename": "/midp/lib/scroll.image_up.png", "start": 44801, "end": 45039}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 45039, "end": 45204}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 45204, "end": 45441}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 45441, "end": 46260}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 46260, "end": 46477}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 46477, "end": 47668}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 47668, "end": 48671}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 48671, "end": 48894}, {"filename": "/midp/lib/indicator_home.png", "start": 48894, "end": 49503}, {"filename": "/midp/lib/doja-support.jar", "start": 49503, "end": 904624}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 904624, "end": 904777}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 904777, "end": 904984}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 904984, "end": 905318}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 905318, "end": 907230}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 907230, "end": 907716}, {"filename": "/midp/lib/indicator_bar.png", "start": 907716, "end": 909661}, {"filename": "/midp/lib/title.image_bg1.png", "start": 909661, "end": 909842}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 909842, "end": 910003}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 910003, "end": 910842}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 910842, "end": 911802}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 911802, "end": 912798}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 912798, "end": 913282}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 913282, "end": 913583}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 913583, "end": 913815}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 913815, "end": 913996}, {"filename": "/midp/lib/skin.bin", "start": 913996, "end": 919772}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 919772, "end": 919939}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 919939, "end": 920152}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 920152, "end": 921049}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 921049, "end": 921385}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 921385, "end": 921562}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 921562, "end": 921781}, {"filename": "/midp/lib/title.image_bg0.png", "start": 921781, "end": 922624}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 922624, "end": 922918}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 922918, "end": 923150}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 923150, "end": 923570}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 923570, "end": 923730}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 923730, "end": 924040}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 924040, "end": 924241}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 924241, "end": 926023}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 926023, "end": 926197}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 926197, "end": 926451}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 926451, "end": 926683}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 926683, "end": 928412}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 928412, "end": 930167}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 930167, "end": 930368}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 930368, "end": 930665}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 930665, "end": 931374}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 931374, "end": 933118}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 933118, "end": 933345}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 933345, "end": 933531}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 933531, "end": 933754}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 933754, "end": 934703}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 934703, "end": 934893}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 934893, "end": 935082}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 935082, "end": 937612}, {"filename": "/midp/lib/trusted_icon.png", "start": 937612, "end": 938404}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 938404, "end": 938590}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 938590, "end": 938858}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 938858, "end": 939046}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 939046, "end": 940557}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 940557, "end": 940720}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 940720, "end": 941021}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 941021, "end": 941177}, {"filename": "/midp/lib/calendar.png", "start": 941177, "end": 941308}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 941308, "end": 941981}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 941981, "end": 942653}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 942653, "end": 942812}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 942812, "end": 942985}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 942985, "end": 945280}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 945280, "end": 946049}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 946049, "end": 946236}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 946236, "end": 946903}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 946903, "end": 947114}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 947114, "end": 947274}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 947274, "end": 948969}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 948969, "end": 949138}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 949138, "end": 949315}, {"filename": "/midp/lib/screen.image_wash.png", "start": 949315, "end": 949477}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 949477, "end": 949813}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 949813, "end": 950738}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 950738, "end": 951465}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 951465, "end": 953176}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 953176, "end": 954970}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 954970, "end": 955283}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 955283, "end": 955662}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 955662, "end": 955833}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 955833, "end": 957220}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 957220, "end": 957395}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 957395, "end": 958247}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 958247, "end": 960334}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 960334, "end": 960541}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 960541, "end": 960711}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 960711, "end": 961792}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 961792, "end": 963353}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 963353, "end": 963573}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 963573, "end": 965345}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 965345, "end": 966640}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 966640, "end": 967963}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 967963, "end": 968182}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 968182, "end": 968918}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 968918, "end": 969212}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 969212, "end": 971116}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 971116, "end": 971327}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 971327, "end": 971697}, {"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 971697, "end": 973597}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 973597, "end": 973798}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 973798, "end": 973972}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 973972, "end": 974195}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 974195, "end": 975108}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 975108, "end": 975315}, {"filename": "/midp/lib/pbar.image_values.png", "start": 975315, "end": 977208}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 977208, "end": 978992}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 978992, "end": 979212}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 979212, "end": 979763}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 979763, "end": 979966}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 979966, "end": 981703}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 981703, "end": 984078}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 984078, "end": 984310}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 984310, "end": 984463}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 984463, "end": 984752}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 984752, "end": 993516}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 993516, "end": 1002241}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 1002241, "end": 1002961}, {"filename": "/midp/appdb/_ack8.png", "start": 1002961, "end": 1006694}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1006694, "end": 1007759}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1007759, "end": 1011095}, {"filename": "/midp/appdb/_dukeok8.png", "start": 1011095, "end": 1015119}, {"filename": "/midp/appdb/_suite8.png", "start": 1015119, "end": 1015911}, {"filename": "/midp/appdb/_main.ks", "start": 1015911, "end": 1016884}, {"filename": "/midp/appdb/_ch_single.png", "start": 1016884, "end": 1020099}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1020099, "end": 1023063}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1023063, "end": 1026365}, {"filename": "/midp/appdb/_single8.png", "start": 1026365, "end": 1027163}], "remote_package_size": 1027163, "package_uuid": "74a56e75-974e-4415-8b78-0969c9d35674"});

  })();
