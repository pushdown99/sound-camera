var fs          = require('fs');
var path        = require('path');
var express     = require('express');
var fileUpload  = require('express-fileupload');
var router      = express.Router();
var app         = express();
var moment      = require('moment-timezone');
var port        = process.env.PORT || 9900;

app.set('views', __dirname + '/views');
app.set('view engine', 'ejs');
app.engine('html', require('ejs').renderFile);

app.use(fileUpload());
app.use(express.json());
app.use(express.urlencoded({ extended: true}));
app.use(express.static(__dirname + '/public'));

/////////////////////////////////////////////////////////////////////////
//
// middleware
//
app.use(function (req, res, next) {
    req.timestamp  = moment().unix();
    req.receivedAt = moment().tz('Asia/Seoul').format('YYYY-MM-DD HH:mm:ss');
    //console.log(req.receivedAt + ': ', req.method, req.protocol +'://' + req.hostname + req.url);
    return next();
  });

//////////////////////////////////////////////////////////////
//
// Express routing
//
app.route('/upload')
  .get(function(req, res, next){
    res.render('upload');
  });

app.route('/')
  .get(function(req, res, next){
    res.render('chart');
  });

app.route('/photo')
  .get(function(req, res, next){
    res.render('photo');
  });

var dat = [0,0,0,0,0,0,0,0]

app.post('/data', function(req, res) {
    dat[req.body.channel] = req.body.peak;
    var f = "/tmp/sound-camera-" + req.body.channel;
    fs.writeFile(f, req.body.data, function(err) {
        if(err) {
            return console.log(err);
        }
    });
    res.sendStatus(200);
  });


var fft = [0,0,0,0,0,0,0,0]

app.post('/fft', function(req, res) {
    fft[req.body.channel] = req.body.freq;
    var f = "/tmp/sound-fft-" + req.body.channel;
    fs.writeFile(f, req.body.data, function(err) {
        if(err) {
            return console.log(err);
        }
    });
    res.sendStatus(200);
  });


app.get('/json-dat', function(req, res) {
    var f = "/tmp/sound-camera-" + req.query.channel;
    var text = fs.readFileSync(f,'utf8')
    res.send (text)
  });

app.get('/json-fft', function(req, res) {
    var f = "/tmp/sound-fft-" + req.query.channel;
    var text = fs.readFileSync(f,'utf8')
    res.send (text)
  });

app.get('/json-dat-val', function(req, res) {
    res.send (JSON.stringify(dat))
  });

app.get('/json-fft-val', function(req, res) {
    res.send (JSON.stringify(fft))
  });

app.post('/upload', function(req, res) {
    if (!req.files || Object.keys(req.files).length === 0) {
        return res.status(400).send('No files were uploaded.');
    }
    let sampleFile = req.files.sampleFile;

    var o = path.resolve(__dirname, './public/data', moment().tz('Asia/Seoul').format('YYYYMMDD-HHmmss') + '.jpg');
    var f = path.resolve(__dirname, './public/data', 'latest' + '.jpg');
    sampleFile.mv(o, function(err) {
      if (err)
        return res.status(500).send(err);

      fs.copyFile(o, f, (err) => {
        if(err) console.log(err);
      });
      res.send('File uploaded!');
    });
});

app.route('/json-photo')
  .get(function(req, res, next){
    const directoryPath = path.join(__dirname, "/public/data/")
    var images = new Array();
    fs.readdir(directoryPath, function(err, files) {
        if (err) {
          console.log("Error getting directory information.")
        } else {
          files.forEach(function(file) {
            var data = new Object() ;
            data.name = file;
            images.push(data);
          })
        }
        res.send(JSON.stringify(images));
        //res.render('noise', {'json': images});
    })
  });


////////////////////////////////////////////////////////
// listener
app.listen(port, function(){
    console.log('Listener: ', 'Example app listening on port ' + port);
});

module.exports = app;
