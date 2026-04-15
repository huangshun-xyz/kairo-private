import UIKit

final class MainView: KRChartView {
    override init(frame: CGRect) {
        super.init(frame: frame)
        candles = [
            KRCandle(open: 102.0, high: 106.0, low: 99.0, close: 104.0, volume: 1200.0),
            KRCandle(open: 104.0, high: 107.0, low: 101.5, close: 103.0, volume: 980.0),
            KRCandle(open: 103.0, high: 108.5, low: 102.0, close: 107.0, volume: 1320.0),
            KRCandle(open: 107.0, high: 109.0, low: 104.5, close: 105.0, volume: 1110.0),
            KRCandle(open: 105.0, high: 110.0, low: 104.0, close: 109.5, volume: 1450.0),
            KRCandle(open: 109.5, high: 111.0, low: 108.0, close: 108.2, volume: 1020.0),
            KRCandle(open: 108.2, high: 112.5, low: 107.0, close: 111.6, volume: 1580.0),
            KRCandle(open: 111.6, high: 114.0, low: 110.0, close: 113.4, volume: 1710.0),
            KRCandle(open: 113.4, high: 115.5, low: 111.8, close: 112.0, volume: 1490.0),
            KRCandle(open: 112.0, high: 116.0, low: 110.5, close: 115.2, volume: 1680.0),
            KRCandle(open: 115.2, high: 118.0, low: 114.0, close: 117.5, volume: 1750.0),
            KRCandle(open: 117.5, high: 119.0, low: 115.8, close: 116.3, volume: 1410.0),
            KRCandle(open: 116.3, high: 120.5, low: 115.5, close: 119.6, volume: 1820.0),
            KRCandle(open: 119.6, high: 121.0, low: 117.2, close: 118.0, volume: 1380.0),
            KRCandle(open: 118.0, high: 122.0, low: 117.0, close: 121.4, volume: 1960.0),
            KRCandle(open: 121.4, high: 123.2, low: 118.5, close: 119.3, volume: 1520.0),
            KRCandle(open: 119.3, high: 124.0, low: 118.0, close: 123.4, volume: 2050.0),
            KRCandle(open: 123.4, high: 126.0, low: 122.2, close: 125.1, volume: 2140.0),
            KRCandle(open: 125.1, high: 127.5, low: 123.8, close: 124.2, volume: 1760.0),
            KRCandle(open: 124.2, high: 128.0, low: 123.5, close: 127.4, volume: 2210.0),
        ]
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}
