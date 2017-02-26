import { enableProdMode } from '@angular/core';
import { platformBrowserDynamic } from '@angular/platform-browser-dynamic';

import { GiantAppModule } from './app/app.module';
import { environment } from './environments/environment';

let afterMain = performance.now();
console.log(`>>> Time to main: ${afterMain - (<any>window).beforeMain} ms`);

if (environment.production) {
  enableProdMode();
}

setTimeout(function () {
  let bootStart = window.performance.now();
  platformBrowserDynamic().bootstrapModule(GiantAppModule).then(() => {
    var bootEnd = window.performance.now();
    console.log(`>>> Bootstrap time: ${bootEnd - bootStart} ms`);
  });
}, 10);
