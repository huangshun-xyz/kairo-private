import QuartzCore
import UIKit

public struct KRCandle {
    public var open: Double
    public var high: Double
    public var low: Double
    public var close: Double
    public var volume: Double

    public init(open: Double, high: Double, low: Double, close: Double, volume: Double) {
        self.open = open
        self.high = high
        self.low = low
        self.close = close
        self.volume = volume
    }
}

open class KRChartView: UIView {
    open override class var layerClass: AnyClass {
        CAMetalLayer.self
    }

    public var candles: [KRCandle] = [] {
        didSet {
            reloadData()
        }
    }

    public var crosshairEnabled: Bool = true {
        didSet {
            bridge.isCrosshairEnabled = crosshairEnabled
            if !crosshairEnabled {
                bridge.clearCrosshair()
            }
            renderChart()
        }
    }

    private let bridge = KRChartBridge()
    private lazy var panRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
    private lazy var pinchRecognizer = UIPinchGestureRecognizer(target: self, action: #selector(handlePinch(_:)))
    private lazy var crosshairRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(handleCrosshair(_:)))
    private var lastPanX: CGFloat = 0
    private var lastPinchScale: CGFloat = 1
    private var visibleRange = (from: 0.0, to: 24.0)

    public override init(frame: CGRect) {
        super.init(frame: frame)
        commonInit()
    }

    public required init?(coder: NSCoder) {
        super.init(coder: coder)
        commonInit()
    }

    open func reloadData() {
        updateVisibleRangeIfNeeded()
        syncCandlesToBridge()
        renderChart()
    }

    open func resetToDefaultPriceVolumeLayout() {
        bridge.resetToDefaultPriceVolumeLayout()
        bridge.isCrosshairEnabled = crosshairEnabled
        reloadData()
    }

    open override func didMoveToWindow() {
        super.didMoveToWindow()
        renderChart()
    }

    open override func layoutSubviews() {
        super.layoutSubviews()
        renderChart()
    }

    private func commonInit() {
        backgroundColor = .white
        isOpaque = true
        contentScaleFactor = UIScreen.main.scale

        bridge.isCrosshairEnabled = crosshairEnabled
        bridge.resetToDefaultPriceVolumeLayout()
        configureGestures()
    }

    private func configureGestures() {
        panRecognizer.maximumNumberOfTouches = 1
        addGestureRecognizer(panRecognizer)

        addGestureRecognizer(pinchRecognizer)

        crosshairRecognizer.minimumPressDuration = 0.15
        addGestureRecognizer(crosshairRecognizer)
    }

    private func makeBridgeCandles(from candles: [KRCandle]) -> [KRCandleValue] {
        candles.map {
            var value = KRCandleValue()
            value.open = $0.open
            value.high = $0.high
            value.low = $0.low
            value.close = $0.close
            value.volume = $0.volume
            return value
        }
    }

    private func syncCandlesToBridge() {
        let bridgeCandles = makeBridgeCandles(from: candles)
        bridgeCandles.withUnsafeBufferPointer { buffer in
            if let baseAddress = buffer.baseAddress {
                bridge.setCandles(baseAddress, count: buffer.count)
            } else {
                var empty = KRCandleValue()
                withUnsafePointer(to: &empty) { pointer in
                    bridge.setCandles(pointer, count: 0)
                }
            }
        }
    }

    private func updateVisibleRangeIfNeeded() {
        if candles.isEmpty {
            visibleRange = (0.0, 24.0)
        } else {
            let count = Double(candles.count)
            visibleRange = (max(0.0, count - 24.0), count)
        }
    }

    private func renderChart() {
        guard !bounds.isEmpty else { return }
        guard let metalLayer = layer as? CAMetalLayer else { return }
        bridge.render(to: metalLayer, size: bounds.size, scale: contentScaleFactor)
    }

    @objc
    private func handlePan(_ recognizer: UIPanGestureRecognizer) {
        let location = recognizer.location(in: self)
        if recognizer.state == .began {
            lastPanX = location.x
            return
        }

        if recognizer.state == .changed {
            let deltaX = location.x - lastPanX
            lastPanX = location.x
            let width = bounds.width
            if width > 0 {
                let viewportWidth = visibleRange.to - visibleRange.from
                let deltaLogical = -(Double(deltaX / width) * viewportWidth)
                visibleRange = (visibleRange.from + deltaLogical, visibleRange.to + deltaLogical)
                bridge.scroll(by: deltaLogical)
                renderChart()
            }
            return
        }

        lastPanX = 0
    }

    @objc
    private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
        if recognizer.state == .began {
            lastPinchScale = recognizer.scale
            return
        }

        if recognizer.state == .changed {
            let deltaScale = recognizer.scale / max(lastPinchScale, 0.01)
            lastPinchScale = recognizer.scale
            let location = recognizer.location(in: self)
            let anchorRatio = bounds.width > 0 ? Float(location.x / bounds.width) : 0.5
            let oldWidth = visibleRange.to - visibleRange.from
            let newWidth = min(max(oldWidth / Double(deltaScale), 5.0), 512.0)
            let anchorLogical = visibleRange.from + Double(anchorRatio) * oldWidth
            visibleRange = (anchorLogical - Double(anchorRatio) * newWidth,
                            anchorLogical + Double(1 - anchorRatio) * newWidth)
            bridge.zoom(by: Double(deltaScale), anchorRatio: anchorRatio)
            renderChart()
            return
        }

        lastPinchScale = 1
    }

    @objc
    private func handleCrosshair(_ recognizer: UILongPressGestureRecognizer) {
        guard crosshairEnabled else { return }

        let location = recognizer.location(in: self)
        switch recognizer.state {
        case .began, .changed:
            bridge.updateCrosshairAt(x: Float(location.x), y: Float(location.y))
            renderChart()
        default:
            bridge.clearCrosshair()
            renderChart()
        }
    }
}
