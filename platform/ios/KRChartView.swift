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

public enum KRScrollBoundaryBehavior {
    case clamp
    case bounce

    fileprivate var hostValue: KRScrollBoundaryBehaviorValue {
        switch self {
        case .clamp:
            return .clamp
        case .bounce:
            return .bounce
        }
    }
}

public struct KRScrollInteractionOptions {
    public var boundaryBehavior: KRScrollBoundaryBehavior
    public var alwaysScrollable: Bool
    public var allowInertia: Bool
    public var dragSensitivity: Double
    public var overscrollFriction: Double
    public var decelerationRate: Double
    public var springMass: Double
    public var springStiffness: Double
    public var springDampingRatio: Double

    public init(
        boundaryBehavior: KRScrollBoundaryBehavior = .bounce,
        alwaysScrollable: Bool = true,
        allowInertia: Bool = true,
        dragSensitivity: Double = 1.0,
        overscrollFriction: Double = 0.52,
        decelerationRate: Double = 0.135,
        springMass: Double = 0.5,
        springStiffness: Double = 100.0,
        springDampingRatio: Double = 1.1
    ) {
        self.boundaryBehavior = boundaryBehavior
        self.alwaysScrollable = alwaysScrollable
        self.allowInertia = allowInertia
        self.dragSensitivity = dragSensitivity
        self.overscrollFriction = overscrollFriction
        self.decelerationRate = decelerationRate
        self.springMass = springMass
        self.springStiffness = springStiffness
        self.springDampingRatio = springDampingRatio
    }

    fileprivate var hostValue: KRScrollInteractionOptionsValue {
        var value = KRScrollInteractionOptionsValue()
        value.boundaryBehavior = boundaryBehavior.hostValue
        value.alwaysScrollable = ObjCBool(alwaysScrollable)
        value.allowInertia = ObjCBool(allowInertia)
        value.dragSensitivity = dragSensitivity
        value.overscrollFriction = overscrollFriction
        value.decelerationRate = decelerationRate
        value.springMass = springMass
        value.springStiffness = springStiffness
        value.springDampingRatio = springDampingRatio
        return value
    }
}

open class KRChartView: UIView, KRIOSChartHostDelegate {
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
            host.isCrosshairEnabled = crosshairEnabled
            if !crosshairEnabled {
                host.clearCrosshair()
            }
            renderChart()
        }
    }

    public var scrollInteractionOptions = KRScrollInteractionOptions() {
        didSet {
            host.setScrollInteractionOptions(scrollInteractionOptions.hostValue)
        }
    }

    private let host = KRIOSChartHost()
    private lazy var panRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
    private lazy var pinchRecognizer = UIPinchGestureRecognizer(target: self, action: #selector(handlePinch(_:)))
    private lazy var crosshairRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(handleCrosshair(_:)))
    private var lastPanX: CGFloat = 0

    public override init(frame: CGRect) {
        super.init(frame: frame)
        commonInit()
    }

    public required init?(coder: NSCoder) {
        super.init(coder: coder)
        commonInit()
    }

    deinit {
        host.setHostActive(false)
    }

    open func reloadData() {
        syncCandlesToHost()
        renderChart()
    }

    open func resetToDefaultPriceVolumeLayout() {
        host.resetToDefaultPriceVolumeLayout()
        host.isCrosshairEnabled = crosshairEnabled
        syncCandlesToHost()
        renderChart()
    }

    open override func didMoveToWindow() {
        super.didMoveToWindow()
        if window == nil {
            host.setHostActive(false)
        } else {
            host.setHostActive(true)
            renderChart()
        }
    }

    open override func layoutSubviews() {
        super.layoutSubviews()
        renderChart()
    }

    private func commonInit() {
        backgroundColor = .white
        isOpaque = true
        contentScaleFactor = UIScreen.main.scale

        host.delegate = self
        host.isCrosshairEnabled = crosshairEnabled
        host.setScrollInteractionOptions(scrollInteractionOptions.hostValue)
        host.resetToDefaultPriceVolumeLayout()
        configureGestures()
    }

    private func configureGestures() {
        panRecognizer.maximumNumberOfTouches = 1
        addGestureRecognizer(panRecognizer)

        addGestureRecognizer(pinchRecognizer)

        crosshairRecognizer.minimumPressDuration = 0.15
        addGestureRecognizer(crosshairRecognizer)
    }

    private func makeHostCandles(from candles: [KRCandle]) -> [KRCandleValue] {
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

    private func syncCandlesToHost() {
        let hostCandles = makeHostCandles(from: candles)
        hostCandles.withUnsafeBufferPointer { buffer in
            if let baseAddress = buffer.baseAddress {
                host.setCandles(baseAddress, count: buffer.count)
            } else {
                var empty = KRCandleValue()
                withUnsafePointer(to: &empty) { pointer in
                    host.setCandles(pointer, count: 0)
                }
            }
        }
    }

    private func renderChart() {
        guard !bounds.isEmpty else { return }
        guard let metalLayer = layer as? CAMetalLayer else { return }
        host.render(to: metalLayer, size: bounds.size, scale: contentScaleFactor)
    }

    @objc
    private func handlePan(_ recognizer: UIPanGestureRecognizer) {
        let location = recognizer.location(in: self)

        switch recognizer.state {
        case .began:
            lastPanX = location.x
            host.handlePanBegan(atX: location.x, y: location.y)

        case .changed:
            let deltaX = location.x - lastPanX
            lastPanX = location.x
            host.handlePanChanged(deltaX: deltaX, deltaY: 0)

        case .ended, .cancelled, .failed:
            let velocityX = recognizer.velocity(in: self).x
            host.handlePanEnded(velocityX: velocityX, velocityY: 0)
            lastPanX = 0

        default:
            break
        }
    }

    @objc
    private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
        switch recognizer.state {
        case .began:
            let location = recognizer.location(in: self)
            let anchorRatio = bounds.width > 0 ? Float(location.x / bounds.width) : 0.5
            host.handlePinchBegan(anchorRatio: anchorRatio)

        case .changed:
            let location = recognizer.location(in: self)
            let anchorRatio = bounds.width > 0 ? Float(location.x / bounds.width) : 0.5
            host.handlePinchChanged(scale: recognizer.scale, anchorRatio: anchorRatio)

        default:
            host.handlePinchEnded()
        }
    }

    @objc
    private func handleCrosshair(_ recognizer: UILongPressGestureRecognizer) {
        guard crosshairEnabled else { return }

        let location = recognizer.location(in: self)
        switch recognizer.state {
        case .began:
            host.handleLongPressBegan(atX: location.x, y: location.y)

        case .changed:
            host.handleLongPressMoved(toX: location.x, y: location.y)

        default:
            host.handleLongPressEnded()
        }
    }

    public func chartHostNeedsDisplay(_ host: KRIOSChartHost) {
        renderChart()
    }
}
