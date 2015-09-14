#include <QTest>
#include "TestUtil.h"

#include "GZip.h"
#include <random>

void fib(int &prev, int &cur)
{
	auto ret = prev + cur;
	prev = cur;
	cur = ret;
}

class GZipTest : public QObject
{
	Q_OBJECT
private
slots:

	void test_Through()
	{
		// test up to 10 MB
		static const int size = 10 * 1024 * 1024;
		QByteArray random;
		QByteArray compressed;
		QByteArray decompressed;
		std::default_random_engine eng((std::random_device())());
		std::uniform_int_distribution<uint8_t> idis(0, std::numeric_limits<uint8_t>::max());

		// initialize random buffer
		for(int i = 0; i < size; i++)
		{
			random.append((char)idis(eng));
		}

		// initialize fibonacci
		int prev = 1;
		int cur = 1;

		// test if fibonacci long random buffers pass through GZip
		do
		{
			QByteArray copy = random;
			copy.resize(cur);
			QVERIFY(GZip::compress(copy, compressed));
			QVERIFY(GZip::decompress(compressed, decompressed));
			QCOMPARE(decompressed, copy);
			fib(prev, cur);
		} while (cur < size);
	}
};

QTEST_GUILESS_MAIN(GZipTest)

#include "tst_GZip.moc"
