
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
    loadPackage({"files": [{"filename": "/midp/lib/alert.image_icon_cnfm.png", "start": 0, "end": 1900}, {"filename": "/midp/lib/screen.image_bg.png", "start": 1900, "end": 2048}, {"filename": "/midp/lib/title.image_bg1.png", "start": 2048, "end": 2229}, {"filename": "/midp/lib/choice.image_btn_icon.png", "start": 2229, "end": 2539}, {"filename": "/midp/lib/stringitem.image_btn3.png", "start": 2539, "end": 2702}, {"filename": "/midp/lib/softbtn.image_bg2.png", "start": 2702, "end": 3438}, {"filename": "/midp/lib/ticker.image_au_bg2.png", "start": 3438, "end": 3661}, {"filename": "/midp/lib/busycrsr.image_frame0.png", "start": 3661, "end": 5940}, {"filename": "/midp/lib/alert.image_icon_warn.png", "start": 5940, "end": 7313}, {"filename": "/midp/lib/menu.image_submenu.png", "start": 7313, "end": 7533}, {"filename": "/midp/lib/softbtn.image_mu_bg2.png", "start": 7533, "end": 7867}, {"filename": "/midp/lib/choice.image_popup_bg3.png", "start": 7867, "end": 8027}, {"filename": "/midp/lib/scroll.image_up.png", "start": 8027, "end": 8265}, {"filename": "/midp/lib/imageitem.image_btn7.png", "start": 8265, "end": 8472}, {"filename": "/midp/lib/alert.image_bg8.png", "start": 8472, "end": 8649}, {"filename": "/midp/lib/busycrsr.image_frame15.png", "start": 8649, "end": 10036}, {"filename": "/midp/lib/dateeditor.image_mon_bg.png", "start": 10036, "end": 10769}, {"filename": "/midp/lib/scroll.image_au_up.png", "start": 10769, "end": 10989}, {"filename": "/midp/lib/stringitem.image_btn4.png", "start": 10989, "end": 11142}, {"filename": "/midp/lib/choice.image_btn_bg6.png", "start": 11142, "end": 11396}, {"filename": "/midp/lib/datefield.image_btn_bg4.png", "start": 11396, "end": 11549}, {"filename": "/midp/lib/dateeditor.image_clock_bg.png", "start": 11549, "end": 14190}, {"filename": "/midp/lib/ticker.image_au_bg1.png", "start": 14190, "end": 14413}, {"filename": "/midp/lib/menu.image_bg2.png", "start": 14413, "end": 14630}, {"filename": "/midp/lib/pbar.image_mtr_full.png", "start": 14630, "end": 16717}, {"filename": "/midp/lib/stringitem.image_lnk.png", "start": 16717, "end": 17666}, {"filename": "/midp/lib/untrusted_icon.png", "start": 17666, "end": 18695}, {"filename": "/midp/lib/dateeditor.image_cal_bg.png", "start": 18695, "end": 23576}, {"filename": "/midp/lib/gauge.image_mtr_empty.png", "start": 23576, "end": 23946}, {"filename": "/midp/lib/title.image_bg2.png", "start": 23946, "end": 24546}, {"filename": "/midp/lib/busycrsr.image_frame8.png", "start": 24546, "end": 26711}, {"filename": "/midp/lib/alert.image_icon_info.png", "start": 26711, "end": 28684}, {"filename": "/midp/lib/softbtn.image_mu_bg1.png", "start": 28684, "end": 28895}, {"filename": "/midp/lib/menu.image_bg6.png", "start": 28895, "end": 29073}, {"filename": "/midp/lib/busycrsr.image_frame14.png", "start": 29073, "end": 30810}, {"filename": "/midp/lib/softbtn.image_au_bg2.png", "start": 30810, "end": 31146}, {"filename": "/midp/lib/gauge.image_dec_btn.png", "start": 31146, "end": 32469}, {"filename": "/midp/lib/pbar.image_values.png", "start": 32469, "end": 34362}, {"filename": "/midp/lib/dateeditor.image_radio0.png", "start": 34362, "end": 35443}, {"filename": "/midp/lib/datefield.image_icon_datetime.png", "start": 35443, "end": 36170}, {"filename": "/midp/lib/gauge.image_mtr_full.png", "start": 36170, "end": 37022}, {"filename": "/midp/lib/title.image_bg0.png", "start": 37022, "end": 37865}, {"filename": "/midp/lib/imageitem.image_btn6.png", "start": 37865, "end": 38159}, {"filename": "/midp/lib/imageitem.image_btn1.png", "start": 38159, "end": 38360}, {"filename": "/midp/lib/busycrsr.image_frame9.png", "start": 38360, "end": 40578}, {"filename": "/midp/lib/ticker.image_bg1.png", "start": 40578, "end": 40810}, {"filename": "/midp/lib/alert.image_bg7.png", "start": 40810, "end": 40996}, {"filename": "/midp/lib/busycrsr.image_frame7.png", "start": 40996, "end": 42981}, {"filename": "/midp/lib/choice.image_btn_bg5.png", "start": 42981, "end": 43141}, {"filename": "/midp/lib/updatebar.image_frame2.png", "start": 43141, "end": 44935}, {"filename": "/midp/lib/stringitem.image_btn6.png", "start": 44935, "end": 45229}, {"filename": "/midp/lib/busycrsr.image_frame5.png", "start": 45229, "end": 46940}, {"filename": "/midp/lib/alert.image_bg3.png", "start": 46940, "end": 47110}, {"filename": "/midp/lib/choice.image_popup_bg1.png", "start": 47110, "end": 47270}, {"filename": "/midp/lib/choice.image_btn_bg8.png", "start": 47270, "end": 47489}, {"filename": "/midp/lib/choice.image_radio0.png", "start": 47489, "end": 48414}, {"filename": "/midp/lib/doja-support.jar", "start": 48414, "end": 930530}, {"filename": "/midp/lib/pti.left_arrow.png", "start": 930530, "end": 930719}, {"filename": "/midp/lib/imageitem.image_btn2.png", "start": 930719, "end": 931032}, {"filename": "/midp/lib/choice.image_popup_bg2.png", "start": 931032, "end": 931243}, {"filename": "/midp/lib/imageitem.image_btn0.png", "start": 931243, "end": 931532}, {"filename": "/midp/lib/menu.image_bg3.png", "start": 931532, "end": 931722}, {"filename": "/midp/lib/choice.image_radio1.png", "start": 931722, "end": 932619}, {"filename": "/midp/lib/datefield.image_btn_bg2.png", "start": 932619, "end": 932808}, {"filename": "/midp/lib/alert.image_bg1.png", "start": 932808, "end": 932973}, {"filename": "/midp/lib/indicator_network.png", "start": 932973, "end": 933493}, {"filename": "/midp/lib/choice.image_popup_bg7.png", "start": 933493, "end": 933653}, {"filename": "/midp/lib/softbtn.image_au_bg0.png", "start": 933653, "end": 933989}, {"filename": "/midp/lib/datefield.image_btn_bg5.png", "start": 933989, "end": 934149}, {"filename": "/midp/lib/ticker.image_bg2.png", "start": 934149, "end": 934381}, {"filename": "/midp/lib/datefield.image_btn_bg1.png", "start": 934381, "end": 934556}, {"filename": "/midp/lib/indicator_home.png", "start": 934556, "end": 935165}, {"filename": "/midp/lib/pbar.image_bg.png", "start": 935165, "end": 936726}, {"filename": "/midp/lib/choice.image_popup_bg8.png", "start": 936726, "end": 936929}, {"filename": "/midp/lib/alert.image_bg0.png", "start": 936929, "end": 937116}, {"filename": "/midp/lib/trusted_icon.png", "start": 937116, "end": 937908}, {"filename": "/midp/lib/datefield.image_btn_bg6.png", "start": 937908, "end": 938115}, {"filename": "/midp/lib/softbtn.image_au_bg1.png", "start": 938115, "end": 938328}, {"filename": "/midp/lib/datefield.image_btn_bg0.png", "start": 938328, "end": 938514}, {"filename": "/midp/lib/stringitem.image_btn8.png", "start": 938514, "end": 938815}, {"filename": "/midp/lib/alert.image_icon_alrm.png", "start": 938815, "end": 940772}, {"filename": "/midp/lib/busycrsr.image_frame3.png", "start": 940772, "end": 942208}, {"filename": "/midp/lib/imageitem.image_btn8.png", "start": 942208, "end": 942509}, {"filename": "/midp/lib/pbar.image_prcnts.png", "start": 942509, "end": 943278}, {"filename": "/midp/lib/scroll.image_au_dn.png", "start": 943278, "end": 943501}, {"filename": "/midp/lib/choice.image_btn_bg4.png", "start": 943501, "end": 943651}, {"filename": "/midp/lib/alert.image_bg6.png", "start": 943651, "end": 943821}, {"filename": "/midp/lib/datefield.image_icon_date.png", "start": 943821, "end": 944307}, {"filename": "/midp/lib/imageitem.image_btn4.png", "start": 944307, "end": 944460}, {"filename": "/midp/lib/imageitem.image_lnk_h.png", "start": 944460, "end": 945409}, {"filename": "/midp/lib/busycrsr.image_frame4.png", "start": 945409, "end": 946899}, {"filename": "/midp/lib/stringitem.image_btn0.png", "start": 946899, "end": 947188}, {"filename": "/midp/lib/choice.image_chkbx0.png", "start": 947188, "end": 948027}, {"filename": "/midp/lib/imageitem.image_btn3.png", "start": 948027, "end": 948190}, {"filename": "/midp/lib/updatebar.image_frame3.png", "start": 948190, "end": 949973}, {"filename": "/midp/lib/busycrsr.image_frame2.png", "start": 949973, "end": 950969}, {"filename": "/midp/lib/dateeditor.image_dates.png", "start": 950969, "end": 954613}, {"filename": "/midp/lib/choice.image_btn_bg1.png", "start": 954613, "end": 954784}, {"filename": "/midp/lib/stringitem.image_btn1.png", "start": 954784, "end": 954985}, {"filename": "/midp/lib/choice.image_popup_bg4.png", "start": 954985, "end": 955141}, {"filename": "/midp/lib/choice.image_popup_bg6.png", "start": 955141, "end": 955310}, {"filename": "/midp/lib/stringitem.image_btn2.png", "start": 955310, "end": 955623}, {"filename": "/midp/lib/menu.image_bg8.png", "start": 955623, "end": 955797}, {"filename": "/midp/lib/menu.image_bg1.png", "start": 955797, "end": 956348}, {"filename": "/midp/lib/datefield.image_btn_bg8.png", "start": 956348, "end": 956536}, {"filename": "/midp/lib/ticker.image_bg0.png", "start": 956536, "end": 956768}, {"filename": "/midp/lib/pti.image_bg0.png", "start": 956768, "end": 957000}, {"filename": "/midp/lib/calendar.png", "start": 957000, "end": 957131}, {"filename": "/midp/lib/NOTICE-JavaMobileAudio.txt", "start": 957131, "end": 957615}, {"filename": "/midp/lib/choice.image_btn_bg2.png", "start": 957615, "end": 957834}, {"filename": "/midp/lib/busycrsr.image_frame6.png", "start": 957834, "end": 959589}, {"filename": "/midp/lib/indicator_bar.png", "start": 959589, "end": 961534}, {"filename": "/midp/lib/busycrsr.image_bg.png", "start": 961534, "end": 964064}, {"filename": "/midp/lib/choice.image_popup_bg0.png", "start": 964064, "end": 964231}, {"filename": "/midp/lib/choice.image_btn_bg0.png", "start": 964231, "end": 964486}, {"filename": "/midp/lib/datefield.image_btn_bg3.png", "start": 964486, "end": 964645}, {"filename": "/midp/lib/busycrsr.image_frame11.png", "start": 964645, "end": 966940}, {"filename": "/midp/lib/busycrsr.image_frame1.png", "start": 966940, "end": 967693}, {"filename": "/midp/lib/gauge.image_values.png", "start": 967693, "end": 968832}, {"filename": "/midp/lib/stringitem.image_btn5.png", "start": 968832, "end": 968997}, {"filename": "/midp/lib/gauge.image_bg.png", "start": 968997, "end": 970692}, {"filename": "/midp/lib/alert.image_bg2.png", "start": 970692, "end": 970879}, {"filename": "/midp/lib/busycrsr.image_frame10.png", "start": 970879, "end": 973254}, {"filename": "/midp/lib/pbar.image_mtr_empty.png", "start": 973254, "end": 975166}, {"filename": "/midp/lib/dateeditor.image_time_bg.png", "start": 975166, "end": 975838}, {"filename": "/midp/lib/menu.image_submenu_hl.png", "start": 975838, "end": 976065}, {"filename": "/midp/lib/busycrsr.image_frame16.png", "start": 976065, "end": 977256}, {"filename": "/midp/lib/choice.image_chkbx1.png", "start": 977256, "end": 978169}, {"filename": "/midp/lib/updatebar.image_frame5.png", "start": 978169, "end": 979951}, {"filename": "/midp/lib/updatebar.image_frame0.png", "start": 979951, "end": 981680}, {"filename": "/midp/lib/busycrsr.image_frame12.png", "start": 981680, "end": 984007}, {"filename": "/midp/lib/ticker.image_au_bg0.png", "start": 984007, "end": 984230}, {"filename": "/midp/lib/alert.image_bg4.png", "start": 984230, "end": 984416}, {"filename": "/midp/lib/datefield.image_icon_time.png", "start": 984416, "end": 985125}, {"filename": "/midp/lib/skin.bin", "start": 985125, "end": 990901}, {"filename": "/midp/lib/datefield.image_btn_bg7.png", "start": 990901, "end": 991068}, {"filename": "/midp/lib/gauge.image_inc_btn.png", "start": 991068, "end": 992363}, {"filename": "/midp/lib/scroll.image_dn.png", "start": 992363, "end": 992600}, {"filename": "/midp/lib/softbtn.image_mu_bg0.png", "start": 992600, "end": 992979}, {"filename": "/midp/lib/softbtn.image_bg1.png", "start": 992979, "end": 993229}, {"filename": "/midp/lib/pti.right_arrow.png", "start": 993229, "end": 993430}, {"filename": "/midp/lib/updatebar.image_frame4.png", "start": 993430, "end": 995202}, {"filename": "/midp/lib/busycrsr.image_frame13.png", "start": 995202, "end": 997106}, {"filename": "/midp/lib/menu.image_bg0.png", "start": 997106, "end": 997526}, {"filename": "/midp/lib/updatebar.image_frame6.png", "start": 997526, "end": 999310}, {"filename": "/midp/lib/indicator_trusted.png", "start": 999310, "end": 1000157}, {"filename": "/midp/lib/dateeditor.image_ampm.png", "start": 1000157, "end": 1000824}, {"filename": "/midp/lib/pti.image_bg1.png", "start": 1000824, "end": 1001056}, {"filename": "/midp/lib/dateeditor.image_radio1.png", "start": 1001056, "end": 1002059}, {"filename": "/midp/lib/choice.image_popup_bg5.png", "start": 1002059, "end": 1002233}, {"filename": "/midp/lib/updatebar.image_frame1.png", "start": 1002233, "end": 1004017}, {"filename": "/midp/lib/updatebar.image_bg.png", "start": 1004017, "end": 1005761}, {"filename": "/midp/lib/menu.image_bg7.png", "start": 1005761, "end": 1006029}, {"filename": "/midp/lib/dateeditor.image_yr_bg.png", "start": 1006029, "end": 1006702}, {"filename": "/midp/lib/alert.image_icon_errr.png", "start": 1006702, "end": 1008213}, {"filename": "/midp/lib/softbtn.image_bg0.png", "start": 1008213, "end": 1009032}, {"filename": "/midp/lib/stringitem.image_btn7.png", "start": 1009032, "end": 1009239}, {"filename": "/midp/lib/menu.image_bg4.png", "start": 1009239, "end": 1009536}, {"filename": "/midp/lib/imageitem.image_lnk_v.png", "start": 1009536, "end": 1010496}, {"filename": "/midp/lib/alert.image_bg5.png", "start": 1010496, "end": 1010673}, {"filename": "/midp/lib/imageitem.image_btn5.png", "start": 1010673, "end": 1010838}, {"filename": "/midp/lib/screen.image_wash.png", "start": 1010838, "end": 1011000}, {"filename": "/midp/lib/choice.image_btn_bg7.png", "start": 1011000, "end": 1011173}, {"filename": "/midp/lib/pti.image_bg2.png", "start": 1011173, "end": 1011405}, {"filename": "/midp/lib/choice.image_btn_bg3.png", "start": 1011405, "end": 1011566}, {"filename": "/midp/lib/menu.image_bg5.png", "start": 1011566, "end": 1011747}, {"filename": "/midp/appdb/_suite8.png", "start": 1011747, "end": 1012539}, {"filename": "/midp/appdb/_ch_fg_requested.png", "start": 1012539, "end": 1013259}, {"filename": "/midp/appdb/_single8.png", "start": 1013259, "end": 1014057}, {"filename": "/midp/appdb/_ch_disabled.png", "start": 1014057, "end": 1017393}, {"filename": "/midp/appdb/_main.ks", "start": 1017393, "end": 1018366}, {"filename": "/midp/appdb/_dukeok8.png", "start": 1018366, "end": 1022390}, {"filename": "/midp/appdb/splash_screen_210x176.png", "start": 1022390, "end": 1031115}, {"filename": "/midp/appdb/_ch_hilight_bg.png", "start": 1031115, "end": 1034079}, {"filename": "/midp/appdb/_ch_suite.png", "start": 1034079, "end": 1037381}, {"filename": "/midp/appdb/_ch_single.png", "start": 1037381, "end": 1040596}, {"filename": "/midp/appdb/splash_screen_176x210.png", "start": 1040596, "end": 1049360}, {"filename": "/midp/appdb/trustedmidlet_icon.png", "start": 1049360, "end": 1050425}, {"filename": "/midp/appdb/_ack8.png", "start": 1050425, "end": 1054158}], "remote_package_size": 1054158, "package_uuid": "46bb12c0-ef16-41c8-858e-53f0651c5556"});

  })();
