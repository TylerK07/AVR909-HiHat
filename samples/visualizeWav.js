const Jimp = require('jimp');
const WAV = require('./debug.js');


console.log('\n');
console.log('------------------------------------------------');
console.log(' DEBUG FILE VISUALIZER                          ');
console.log('------------------------------------------------');
console.log('Creating decompressed visualzation in: debug.png');
console.log('\n');


let SAMPLE_SIZE = 32768;
let sampCounter = 0;
let sampOffset  = 0;
let x = 0;

let image = new Jimp(SAMPLE_SIZE, 64, function (err, image) {
  if (err) throw err;

  let output = "";

  while( sampCounter < SAMPLE_SIZE ){
    let s1 = sample[ sampCounter ];
    let s2 = sample[ sampCounter + 1 ];
    let s = 0;

    switch (sampOffset) {
      case 0:
        s = s1 & 0xFC;    // This is where the issue was. There must be stuff in the bottom two bits of the samples here
        sampOffset = 6;
        break;
      case 2:
        s = (s1 << 2) & 0xFF;
        sampOffset = 0;
        sampCounter++;
        break;
      case 4:
        s = ((s1 << 4) & 0xFF) | ((s2 >> 4) & 0xFC);
        sampOffset = 2;
        sampCounter++;
        break;
      case 6:
        s = ((s1 << 6) & 0xFF) | ((s2 >> 2) & 0xFC);
        /*
        console.log( "s1 << 6:" + ((s1<<6) & 0xFF) );
        console.log( "s2 >> 2:" + (s2>>>2) );
        console.log( "(s2 >> 2) & 0xFC:" + ((s2>>>2) & 0xFC) );
        */
        sampOffset = 4;
        sampCounter++;
        break;
    }
    //output += (s>>>2) + " ";
    
	image.setPixelColor(Jimp.rgbaToInt(0x00, 0xFF, 0xFF, 0xFF), x, s>>>2);
    
    x++;
  }
console.log( output );

  image.write('debug.png', (err) => {
    if (err) throw err;
  });
});