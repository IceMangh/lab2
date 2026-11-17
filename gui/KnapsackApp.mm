#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "Knapsack.h"
#include "StructureLab.h"
#include <algorithm>
#include <chrono>
#include <vector>

static NSDictionary* RunAlgorithm(const DynamicArray<Item>& items, int capacity, bool optimized) {
    std::vector<double> times;
    KnapsackResult result;
    for (int i = 0; i < 5; ++i) {
        auto start = std::chrono::steady_clock::now();
        KnapsackResult current = optimized
            ? SolveOptimized(items, capacity)
            : SolveBruteForce(items, capacity);
        auto end = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
        result = current;
    }
    std::sort(times.begin(), times.end());
    NSMutableArray* indices = [NSMutableArray array];
    for (int i = 0; i < result.itemIndices.GetLength(); ++i) {
        [indices addObject:@(result.itemIndices[i])];
    }
    return @{@"cost": @(result.totalCost), @"volume": @(result.usedVolume),
             @"indices": indices, @"timeMs": @(times[2]), @"repetitions": @5};
}

static NSDictionary* SolveRequest(NSDictionary* request) {
    NSArray* input = request[@"items"];
    int capacity = [request[@"capacity"] intValue];
    NSString* method = request[@"method"];
    BOOL showTrace = [request[@"action"] isEqualToString:@"trace"];
    if (![input isKindOfClass:[NSArray class]] || input.count > 200 || capacity < 0) {
        throw std::invalid_argument("Некорректные исходные данные");
    }
    if (![@[@"optimized", @"brute", @"compare"] containsObject:method]) {
        throw std::invalid_argument("Неизвестный метод поиска");
    }
    DynamicArray<Item> items(static_cast<int>(input.count));
    for (int i = 0; i < items.GetLength(); ++i) {
        NSDictionary* item = input[i];
        long long volume = [item[@"volume"] longLongValue];
        long long value = [item[@"value"] longLongValue];
        if (volume < 0 || volume > INT_MAX || value < INT_MIN || value > INT_MAX) {
            throw std::invalid_argument("Объём и стоимость должны быть целыми числами в диапазоне int");
        }
        items[i] = Item(static_cast<int>(volume), static_cast<int>(value));
    }
    if ([method isEqualToString:@"brute"] && items.GetLength() > 28) {
        throw std::invalid_argument("Для полного перебора выберите не более 28 предметов");
    }
    if (showTrace) {
        if ([method isEqualToString:@"compare"]) {
            throw std::invalid_argument("Для показа шагов выберите один алгоритм");
        }
        SearchTrace trace;
        KnapsackResult result = [method isEqualToString:@"optimized"]
            ? SolveOptimized(items, capacity, &trace)
            : SolveBruteForce(items, capacity, &trace);
        const char* actions[] = {"start", "visit", "best", "skip", "take", "capacity",
                                 "duplicate", "bound", "stale", "leaf", "backtrack", "finish"};
        NSMutableArray* steps = [NSMutableArray array];
        for (const SearchStep& step : trace.steps) {
            NSMutableArray* chosen = [NSMutableArray array];
            for (int index : step.chosen) [chosen addObject:@(index)];
            [steps addObject:@{
                @"type": [NSString stringWithUTF8String:actions[static_cast<int>(step.action)]],
                @"node": @(step.nodeId), @"parent": @(step.parentId), @"take": @(step.take),
                @"item": @(step.itemIndex < items.GetLength() ? step.itemIndex : -1),
                @"index": @(step.state.index), @"volume": @(step.state.usedVolume),
                @"cost": @(step.state.totalCost), @"bestCost": @(step.bestCost),
                @"bound": trace.optimized ? @(step.bound) : NSNull.null,
                @"storedCost": @(step.storedCost), @"path": chosen,
                @"number": @(step.number), @"visited": @(step.visited),
                @"pruned": @(step.pruned), @"pending": trace.optimized ? @(step.pending) : NSNull.null
            }];
        }
        NSMutableArray* indices = [NSMutableArray array];
        for (int i = 0; i < result.itemIndices.GetLength(); ++i) [indices addObject:@(result.itemIndices[i])];
        return @{@"steps": steps, @"method": method, @"truncated": @(trace.truncated),
                 @"totalEvents": @(trace.totalEvents), @"visited": @(trace.visited), @"pruned": @(trace.pruned),
                 @"limit": @(trace.limit),
                 @"result": @{@"cost": @(result.totalCost), @"volume": @(result.usedVolume), @"indices": indices}};
    }
    NSMutableDictionary* results = [NSMutableDictionary dictionary];
    if (![method isEqualToString:@"brute"]) {
        results[@"optimized"] = RunAlgorithm(items, capacity, true);
    }
    if (![method isEqualToString:@"optimized"]) {
        if (items.GetLength() <= 28) {
            results[@"brute"] = RunAlgorithm(items, capacity, false);
        } else {
            results[@"brute"] = @{@"skipped": @YES};
        }
    }
    if (results[@"optimized"] && results[@"brute"] && !results[@"brute"][@"skipped"]
        && ![results[@"optimized"][@"cost"] isEqual:results[@"brute"][@"cost"]]) {
        throw std::runtime_error("Алгоритмы вернули разные ответы");
    }
    return results;
}

@interface KnapsackApp : NSObject <NSApplicationDelegate, WKScriptMessageHandler, WKNavigationDelegate>
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) WKWebView* webView;
@property(nonatomic, assign) BOOL busy;
@end

@implementation KnapsackApp
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSMenu* menu = [[NSMenu alloc] init];
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menu addItem:appItem];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"Рюкзак"];
    [appMenu addItemWithTitle:@"Завершить Рюкзак" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;
    NSMenuItem* editItem = [[NSMenuItem alloc] init];
    [menu addItem:editItem];
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Правка"];
    [editMenu addItemWithTitle:@"Отменить" action:@selector(undo:) keyEquivalent:@"z"];
    [editMenu addItemWithTitle:@"Вырезать" action:@selector(cut:) keyEquivalent:@"x"];
    [editMenu addItemWithTitle:@"Копировать" action:@selector(copy:) keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Вставить" action:@selector(paste:) keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Выбрать всё" action:@selector(selectAll:) keyEquivalent:@"a"];
    editItem.submenu = editMenu;
    NSApp.mainMenu = menu;

    NSRect screen = NSScreen.mainScreen.visibleFrame;
    CGFloat width = MIN(1460, screen.size.width - 70);
    CGFloat height = MIN(970, screen.size.height - 60);
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"Задача о рюкзаке";
    self.window.minSize = NSMakeSize(980, 720);
    self.window.titlebarAppearsTransparent = YES;
    self.window.backgroundColor = [NSColor colorWithRed:0.969 green:0.973 blue:0.965 alpha:1];
    [self.window center];

    WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
    [configuration.userContentController addScriptMessageHandler:self name:@"knapsack"];
    self.webView = [[WKWebView alloc] initWithFrame:self.window.contentView.bounds configuration:configuration];
    self.webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.webView.navigationDelegate = self;
    [self.webView setValue:@NO forKey:@"drawsBackground"];
    self.window.contentView = self.webView;
    NSURL* folder = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"web"];
    [self.webView loadFileURL:[folder URLByAppendingPathComponent:@"index.html"] allowingReadAccessToURL:folder];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)action
    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    decisionHandler(action.request.URL.isFileURL ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel);
}

- (void)sendResponse:(NSDictionary*)response {
    NSData* data = [NSJSONSerialization dataWithJSONObject:response options:0 error:nil];
    NSString* json = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSString* script = [NSString stringWithFormat:@"window.onNativeMessage(%@)", json];
    [self.webView evaluateJavaScript:script completionHandler:nil];
}

- (void)userContentController:(WKUserContentController*)controller didReceiveScriptMessage:(WKScriptMessage*)message {
    if (!message.frameInfo.isMainFrame || ![message.body isKindOfClass:[NSDictionary class]]) {
        return;
    }
    NSDictionary* request = message.body;
    NSNumber* requestId = request[@"id"];
    if ([request[@"action"] isEqualToString:@"export"]) {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.nameFieldStringValue = [request[@"name"] isEqualToString:@"structure-tests.csv"]
            ? @"structure-tests.csv" : @"knapsack-results.csv";
        [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
            if (response == NSModalResponseOK) {
                NSError* error = nil;
                BOOL saved = [request[@"csv"] writeToURL:panel.URL atomically:YES encoding:NSUTF8StringEncoding error:&error];
                [self sendResponse:@{@"id": requestId, @"saved": @(saved),
                                     @"error": error.localizedDescription ?: @""}];
            } else {
                [self sendResponse:@{@"id": requestId, @"cancelled": @YES}];
            }
        }];
        return;
    }
    if ((![@[@"solve", @"trace", @"structure"] containsObject:request[@"action"]]) || self.busy) {
        [self sendResponse:@{@"id": requestId, @"error": @"Дождитесь завершения текущего расчёта"}];
        return;
    }
    self.busy = YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            NSDictionary* response;
            try {
                if ([request[@"action"] isEqualToString:@"structure"]) {
                    response = @{@"id": requestId, @"lab": RunStructureLab(request)};
                } else {
                    NSString* key = [request[@"action"] isEqualToString:@"trace"] ? @"trace" : @"results";
                    response = @{@"id": requestId, key: SolveRequest(request)};
                }
            } catch (const std::exception& error) {
                response = @{@"id": requestId, @"error": [NSString stringWithUTF8String:error.what()]};
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                self.busy = NO;
                [self sendResponse:response];
            });
        }
    });
}
@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = NSApplication.sharedApplication;
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        KnapsackApp* delegate = [[KnapsackApp alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
