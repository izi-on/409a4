import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.RecursiveTask;

public class q1 extends RecursiveTask<String> {

    String x,y;

    q1(String x, String y) {
        this.x = x;
        this.y = y;
    }

    protected String compute() {
        int numOfXDigits = x.length();
        int numOfYDigits = y.length();

        //base case
        if (numOfXDigits == 1 && numOfYDigits == 1) {
            return Integer.toString(Integer.parseInt(x) * Integer.parseInt(y));
        }

        //padding ensures recursive calls are made on numbers of equal length
        if (numOfXDigits < numOfYDigits) {
            x = pad(x, numOfYDigits);
            numOfXDigits = numOfYDigits;
        } else if (numOfYDigits < numOfXDigits) {
            y = pad(y, numOfXDigits);
            numOfYDigits = numOfXDigits;
        }

        int i = numOfXDigits / 2; // value of i at which the digits are split
        String xH = this.x.substring(0, i);
        String xL = this.x.substring(i, numOfXDigits);
        String yH = this.y.substring(0, i);
        String yL = this.y.substring(i, numOfYDigits);
//        System.out.println("xH, xL, yH and yL:" + xH + ", " + xL + ", " + yH + ", " + yL);
        q1 computeA = new q1(xH, yH);
        q1 computeB = new q1(xL, yL);
        q1 computeCMultiplication = new q1(add(xH, xL), add(yH, yL));

        computeA.fork();
        computeCMultiplication.fork();
        String b = computeB.compute();
        String a = computeA.join();
        String c = sub(sub(computeCMultiplication.join(), a), b);
        return prune(add(add(a + pad("", 2 * (numOfXDigits - i)), c + pad("", numOfXDigits - i)), b));
    }

    // returns the sum of 2 base-10 integers expressed as non-empty strings, perhaps with a leading "-".
    // e.g., add("0010","-9301") returns "-9291"
    // nb: returned string may have excess leading 0s.
    public static String add(String x,String y) {
//        System.out.println("Adding, " +  x + ", and " + y);
        String r = "";
        if (x.charAt(0)=='-') {
            if (y.charAt(0)=='-') {
                // -x + -y === - (x+y)
                r = '-' + add(x.substring(1),y.substring(1));
                return r;
            }
            // -x + y === y - x
            r = sub(y,x.substring(1));
            return r;
        } else if (y.charAt(0)=='-') {
            // x + -y === x - y
            r = sub(x,y.substring(1));
            return r;
        }

        // can assume both positive here

        // make sure same length
        int slen = x.length();
        if (y.length()!=slen) {
            slen = (y.length() > slen) ? y.length() : slen;
            x = pad(x,slen);
            y = pad(y,slen);
        }
        int carry = 0;
        for (int i=x.length()-1;i>=0;i--) {
            int sum = Character.getNumericValue(x.charAt(i))+Character.getNumericValue(y.charAt(i))+carry;
            if (sum>=10) {
                sum -= 10;
                carry = 1;
            } else {
                carry = 0;
            }
            r = sum + r;
        }
        if (carry!=0)
            r = "1" + r;
        return r;
    }

    // returns the difference between 2 base-10 integers expressed as non-empty strings, perhaps with a leading "-".
    // e.g., sub("0010","-9301") returns "9311"
    // nb: returned string may have excess leading 0s.
    static String sub(String x,String y) {
        String r = "";
        if (x.charAt(0)=='-') {
            if (y.charAt(0)=='-') {
                // -x - -y  === -x + y  === y - x
                r = sub(y.substring(1),x.substring(1));
                return r;
            }
            // -x - y === - (x+y)
            r = add(x.substring(1),y);
            if (r.length()>0 && r.charAt(0)!='-')
                r = "-" + r;
            return r;
        } else if (y.charAt(0)=='-') {
            // x - -y === x + y
            r = add(x,y.substring(1));
            return r;
        }

        int slen = x.length();
        if (y.length()!=slen) {
            slen = (y.length() > slen) ? y.length() : slen;
            x = pad(x,slen);
            y = pad(y,slen);
        }
        int borrow = 0;
        for (int i=x.length()-1;i>=0;i--) {
            int diff = Character.getNumericValue(x.charAt(i))-borrow-Character.getNumericValue(y.charAt(i));
            //System.out.println("sum of "+x.charAt(i)+"+"+y.charAt(i)+"+"+carry+" = "+sum);
            if (diff<0) {
                borrow = 1;
                diff += 10;
            } else {
                borrow = 0;
            }
            r = diff + r;
        }
        if (borrow!=0) { // flip it around and try again
            r = "-"+sub(y,x);
        }
        return r;
    }

    // remove unnecessary leading 0s from a base-10 number expressed as a string.
    public static String prune(String s) {
        if (s.charAt(0)=='-') return "-"+prune(s.substring(1));
        s = s.replaceFirst("^00*","");
        if (s.length()==0) return "0";
        return s;
    }

    // add leading 0s to a base-10 number expressed as a string to ensure the string
    // is of the length given.
    // nb: assumes a positive number input.
    public static String pad(String s,int n) {
        return String.format("%"+n+"s",s).replace(' ','0');
    }


    // note: to test very large number inputs (on linux), invoke as follows
    // (replacing 10000 with as many digits as you want):
    // java q1 `tr -dc "[:digit:]" < /dev/urandom | head -c 10000` `tr -dc "[:digit:]" < /dev/urandom | head -c 10000`
    public static void main(String[] args) {
        //read 3 args params (all ints)
        int t = Integer.parseInt(args[0]);
        String x = args[1];
        String y = args[2];


        ForkJoinPool pool = new ForkJoinPool(t);
        q1 q = new q1(x, y);

        long startTime = System.nanoTime();

        String res = pool.invoke(q);

        long endTime = System.nanoTime();
        double durationInSeconds = (double) (endTime - startTime) / 1_000_000_000.0;
        System.out.println(res);
        System.out.println("Execution time in seconds: " + durationInSeconds);
    }
}